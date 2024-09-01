// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "Queue.hxx"
#include "gidle.hxx"
#include "TagMask.hxx"
#include "event/FineTimerEvent.hxx"

#ifdef ENABLE_ASYNC_CONNECT
#include "aconnect.hxx"
#endif

#include <mpd/client.h> // IWYU pragma: export

#include <string>

struct AsyncMpdConnect;

struct mpdclient final
	: MpdIdleHandler
#ifdef ENABLE_ASYNC_CONNECT
	, AsyncMpdConnectHandler
#endif
{
	EventLoop &event_loop;

#ifdef ENABLE_ASYNC_CONNECT
	/**
	 * These settings are used to connect to MPD asynchronously.
	 */
	struct mpd_settings *settings;

#ifndef _WIN32
	/**
	 * A second set of settings, just in case #settings did not
	 * work.  This is only used if #settings refers to a local
	 * socket path, and this one is supposed to be a fallback to
	 * IP on the default port (6600).
	 */
	struct mpd_settings *settings2;
#endif

#else
	const char *host;
	unsigned port;
#endif

	const unsigned timeout_ms;

	const char *const password;

	/* playlist */
	MpdQueue playlist;

#ifdef ENABLE_ASYNC_CONNECT
	AsyncMpdConnect *async_connect = nullptr;
#endif

	struct mpd_connection *connection = nullptr;

	/**
	 * Tracks idle events.  It is automatically called by
	 * mpdclient_get_connection().
	 */
	MpdIdleSource *source = nullptr;

	struct mpd_status *status = nullptr;
	const struct mpd_song *current_song = nullptr;

	/**
	 * A timer which re-enters MPD idle mode before the next main
	 * loop iteration.
	 */
	FineTimerEvent enter_idle_timer;

	/**
	 * This attribute is incremented whenever the connection changes
	 * (i.e. on disconnection and (re-)connection).
	 */
	unsigned connection_id = 0;

	int volume = -1;

	/**
	 * A bit mask of idle events occurred since the last update.
	 */
	unsigned events = 0;

	enum mpd_state state = MPD_STATE_UNKNOWN;

	bool enable_tag_whitelist = false;
	TagMask tag_whitelist;

#if defined(ENABLE_ASYNC_CONNECT) && !defined(_WIN32)
	bool connecting2;
#endif

	/**
	 * This attribute is true when the connection is currently in
	 * "idle" mode, and the #mpd_glib_source waits for an event.
	 */
	bool idle = false;

	/**
	 * Is MPD currently playing?
	 */
	bool playing = false;

	/**
	 * Is MPD currently playing or paused?
	 */
	bool playing_or_paused = false;

	mpdclient(EventLoop &_event_loop,
		  const char *host, unsigned port,
		  unsigned _timeout_ms, const char *_password);

	~mpdclient() noexcept {
		Disconnect();

#ifdef ENABLE_ASYNC_CONNECT
		mpd_settings_free(settings);

#ifndef _WIN32
		if (settings2 != nullptr)
			mpd_settings_free(settings2);
#endif
#endif
	}

	auto &GetEventLoop() const noexcept {
		return event_loop;
	}

#ifdef ENABLE_ASYNC_CONNECT

	const struct mpd_settings &GetSettings() const noexcept {
#ifndef _WIN32
		if (connecting2)
			return *settings2;
#endif

		return *settings;
	}

#endif

	/**
	 * Determine a human-readable "name" of the settings currently used to
	 * connect to MPD.
	 *
	 * @return an allocated string that needs to be freed (with g_free())
	 * by the caller
	 */
	std::string GetSettingsName() const noexcept;

	void WhitelistTags(TagMask mask) noexcept;

	bool IsConnected() const noexcept {
		return connection != nullptr;
	}

	/**
	 * Is this object "dead"?  i.e. not connected and not
	 * currently doing anything to connect.
	 */
	[[gnu::pure]]
	bool IsDead() const noexcept {
		return connection == nullptr
#ifdef ENABLE_ASYNC_CONNECT
			&& async_connect == nullptr
#endif
			;
	}

	[[gnu::pure]]
	int GetCurrentSongId() const noexcept {
		return status != nullptr
			? mpd_status_get_song_id(status)
			: -1;
	}

	[[gnu::pure]]
	int GetCurrentSongPos() const noexcept {
		return status != nullptr
			? mpd_status_get_song_pos(status)
			: -1;
	}

	/**
	 * Returns the song that is "current".  This can be valid even
	 * if MPD is not playing.
	 */
	[[gnu::pure]]
	const struct mpd_song *GetCurrentSong() const noexcept {
		return current_song;
	}

	[[gnu::pure]]
	int GetPlayingSongId() const noexcept {
		return playing_or_paused
			? GetCurrentSongId()
			: -1;
	}

	/**
	 * Returns the song that is currently being played (or
	 * paused).
	 */
	[[gnu::pure]]
	const struct mpd_song *GetPlayingSong() const noexcept {
		return playing_or_paused
			? GetCurrentSong()
			: nullptr;
	}

	void Connect() noexcept;

	void Disconnect() noexcept;

	/**
	 * @return true if the cause has been fixed (e.g. by sending a
	 * password) and the caller may retry the operation
	 */
	bool HandleError() noexcept;

	/**
	 * Like HandleError(), but called from inside the auth
	 * callback, and avoids recursion into the auth callback.
	 */
	void HandleAuthError() noexcept;

	struct mpd_connection *GetConnection() noexcept;

	bool FinishCommand() noexcept {
		if (mpd_response_finish(connection))
			return true;

		HandleError();
		return false;
	}

	bool Update() noexcept;

	bool OnConnected(struct mpd_connection *_connection) noexcept;

	const struct mpd_status *ReceiveStatus() noexcept;

	template<typename F>
	bool WithConnection(F &&f) noexcept {
		while (true) {
			auto *c = GetConnection();
			if (c == nullptr)
				return false;

			if (f(*c))
				return true;

			enum mpd_error error = mpd_connection_get_error(c);
			if (error == MPD_ERROR_SUCCESS)
				return false;

			if (!HandleError())
				return false;
		}
	}

	bool RunVolume(unsigned new_volume) noexcept;
	bool RunVolumeUp() noexcept;
	bool RunVolumeDown() noexcept;

	bool RunClearQueue() noexcept;
	bool RunAdd(const struct mpd_song &song) noexcept;
	bool RunDelete(unsigned pos) noexcept;
	bool RunDeleteRange(unsigned start, unsigned end) noexcept;
	bool RunMove(unsigned dest, unsigned src) noexcept;

private:
#ifdef ENABLE_ASYNC_CONNECT
	void StartConnect(const struct mpd_settings &s) noexcept;
#endif

	void InvokeErrorCallback() noexcept;

	bool UpdateQueue() noexcept;
	bool UpdateQueueChanges() noexcept;

	void ClearStatus() noexcept;

	void ScheduleEnterIdle() noexcept {
		enter_idle_timer.Schedule(std::chrono::milliseconds(10));
	}

	void CancelEnterIdle() noexcept {
		enter_idle_timer.Cancel();
	}

	void OnEnterIdleTimer() noexcept;

#ifdef ENABLE_ASYNC_CONNECT
	/* virtual methods from AsyncMpdConnectHandler */
	void OnAsyncMpdConnect(struct mpd_connection *c) noexcept override;
	void OnAsyncMpdConnectError(std::exception_ptr e) noexcept override;
#endif

	/* virtual methods from MpdIdleHandler */
	void OnIdle(unsigned events) noexcept override;
	void OnIdleError(enum mpd_error error,
			 enum mpd_server_error server_error,
			 const char *message) noexcept override;
};

/**
 * All idle events the version of libmpdclient, ncmpc is compiled
 * against, supports.
 */
static constexpr unsigned MPD_IDLE_ALL = MPD_IDLE_DATABASE
	| MPD_IDLE_STORED_PLAYLIST
	| MPD_IDLE_QUEUE
	| MPD_IDLE_PLAYER
	| MPD_IDLE_MIXER
	| MPD_IDLE_OUTPUT
	| MPD_IDLE_OPTIONS
	| MPD_IDLE_UPDATE
	| MPD_IDLE_STICKER
	| MPD_IDLE_SUBSCRIPTION
	| MPD_IDLE_MESSAGE;

/*** MPD Commands  **********************************************************/

bool
mpdclient_cmd_crop(struct mpdclient &c) noexcept;

bool
mpdclient_cmd_clear(struct mpdclient &c) noexcept;

bool
mpdclient_cmd_add_path(struct mpdclient &c, const char *path) noexcept;

bool
mpdclient_cmd_subscribe(struct mpdclient &c, const char *channel) noexcept;

bool
mpdclient_cmd_unsubscribe(struct mpdclient &c, const char *channel) noexcept;

bool
mpdclient_cmd_send_message(struct mpdclient &c, const char *channel,
			   const char *text) noexcept;
