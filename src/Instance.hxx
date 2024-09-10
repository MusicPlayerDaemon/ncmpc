// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "AsyncUserInput.hxx"
#include "UserInputHandler.hxx"
#include "DelayedSeek.hxx"
#include "screen.hxx"
#include "client/mpdclient.hxx"
#include "event/Loop.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "event/FineTimerEvent.hxx"

#ifdef ENABLE_LIRC
#include "lirc.hxx"
#endif

/**
 * A singleton holding global instance variables.
 */
class Instance final : MpdClientHandler, UserInputHandler {
	EventLoop event_loop;

	struct mpdclient client;

	DelayedSeek seek;

	/**
	 * This timer is installed when the connection to the MPD
	 * server is broken.  It tries to recover by reconnecting
	 * periodically.
	 */
	CoarseTimerEvent reconnect_timer;

	FineTimerEvent update_timer;
	bool pending_update_timer = false;

#ifndef NCMPC_MINI
	CoarseTimerEvent check_key_bindings_timer;
#endif

	ScreenManager screen_manager;

#ifdef ENABLE_LIRC
	LircInput lirc_input;
#endif

	AsyncUserInput user_input;

public:
	Instance();
	~Instance();

	Instance(const Instance &) = delete;
	Instance &operator=(const Instance &) = delete;

	auto &GetClient() {
		return client;
	}

	auto &GetSeek() noexcept {
		return seek;
	}

	auto &GetScreenManager() {
		return screen_manager;
	}

	void Quit() noexcept {
		event_loop.Break();
	}

	void UpdateClient() noexcept;

	void Run();

	template<typename D>
	void ScheduleReconnect(const D &expiry_time) {
		reconnect_timer.Schedule(expiry_time);
	}

	void EnableUpdateTimer() noexcept {
		if (!pending_update_timer)
			ScheduleUpdateTimer();
	}

	void DisableUpdateTimer() noexcept {
		if (pending_update_timer) {
			pending_update_timer = false;
			update_timer.Cancel();
		}
	}

#ifndef NCMPC_MINI
	void ScheduleCheckKeyBindings() noexcept {
		check_key_bindings_timer.Schedule(std::chrono::seconds(10));
	}
#endif

private:
#ifndef _WIN32
	void InitSignals();
	void OnSigwinch() noexcept;
#endif

	void OnReconnectTimer() noexcept;

	void OnUpdateTimer() noexcept;

	void ScheduleUpdateTimer() noexcept {
		pending_update_timer = true;
		update_timer.Schedule(std::chrono::milliseconds(500));
	}

#ifndef NCMPC_MINI
	void OnCheckKeyBindings() noexcept;
#endif

	// virtual methods from Mpdclient
	void OnMpdConnected() noexcept override;
	void OnMpdConnectFailed() noexcept override;
	void OnMpdConnectionLost() noexcept override;
	void OnMpdError(std::string_view message) noexcept override;
	void OnMpdError(std::exception_ptr e) noexcept override;
	bool OnMpdAuth() noexcept override;
	void OnMpdIdle(unsigned events) noexcept override;

	// virtual methods from UserInputHandler
	bool OnRawKey(int key) noexcept override;
	bool OnCommand(Command cmd) noexcept override;
#ifdef HAVE_GETMOUSE
	bool OnMouse(Point p, mmask_t bstate) noexcept override;
#endif
	bool CancelModalDialog() noexcept override;
};
