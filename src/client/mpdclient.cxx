// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "mpdclient.hxx"
#include "config.h"
#include "gidle.hxx"
#include "charset.hxx"

#include <mpd/client.h>

#include <assert.h>

void
mpdclient::OnEnterIdleTimer() noexcept
{
	assert(source != nullptr);
	assert(!idle);

	if (authenticating)
		return;

	idle = source->Enter();
}

static void
mpdclient_invoke_error_callback(MpdClientHandler &handler,
				enum mpd_error error,
				const char *message) noexcept
{
	if (error == MPD_ERROR_SERVER)
		/* server errors are UTF-8, the others are locale */
		handler.OnMpdError(Utf8ToLocale{message});
	else
		handler.OnMpdError(message);
}

void
mpdclient::InvokeErrorCallback() noexcept
{
	assert(connection != nullptr);

	enum mpd_error error = mpd_connection_get_error(connection);
	assert(error != MPD_ERROR_SUCCESS);

	mpdclient_invoke_error_callback(handler, error,
					mpd_connection_get_error_message(connection));
}

void
mpdclient::OnIdle(unsigned _events) noexcept
{
	assert(IsReady());

	idle = false;

	events |= _events;
	Update();

	handler.OnMpdIdle(events);
	events = 0;

	if (source != nullptr)
		ScheduleEnterIdle();
}

void
mpdclient::OnIdleError(enum mpd_error error,
		       enum mpd_server_error,
		       const char *message) noexcept
{
	assert(IsReady());

	idle = false;

	mpdclient_invoke_error_callback(handler, error, message);
	Disconnect();
	handler.OnMpdConnectionLost();
}

/****************************************************************************/
/*** mpdclient functions ****************************************************/
/****************************************************************************/

bool
mpdclient::HandleError() noexcept
{
	enum mpd_error error = mpd_connection_get_error(connection);
	assert(error != MPD_ERROR_SUCCESS);

	if (error == MPD_ERROR_SERVER &&
	    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_PERMISSION)
		return handler.OnMpdAuth();

	mpdclient_invoke_error_callback(handler, error,
					mpd_connection_get_error_message(connection));

	if (!mpd_connection_clear_error(connection)) {
		Disconnect();
		handler.OnMpdConnectionLost();
	}

	return false;
}

void
mpdclient::HandleAuthError() noexcept
{
	enum mpd_error error = mpd_connection_get_error(connection);
	assert(error != MPD_ERROR_SUCCESS);

	mpdclient_invoke_error_callback(handler, error,
					mpd_connection_get_error_message(connection));

	if (!mpd_connection_clear_error(connection)) {
		Disconnect();
		handler.OnMpdConnectionLost();
	}
}

#ifdef ENABLE_ASYNC_CONNECT
#ifndef _WIN32

static bool
is_local_socket(const char *host) noexcept
{
	return *host == '/' || *host == '@';
}

static bool
settings_is_local_socket(const struct mpd_settings *settings) noexcept
{
	const char *host = mpd_settings_get_host(settings);
	return host != nullptr && is_local_socket(host);
}

#endif
#endif

mpdclient::mpdclient(EventLoop &event_loop,
		     const char *_host, unsigned _port,
		     unsigned _timeout_ms, const char *_password,
		     MpdClientHandler &_handler)
	:handler(_handler),
	 timeout_ms(_timeout_ms), password(_password),
	 enter_idle_timer(event_loop, BIND_THIS_METHOD(OnEnterIdleTimer))
{
#ifdef ENABLE_ASYNC_CONNECT
	settings = mpd_settings_new(_host, _port, _timeout_ms,
				    nullptr, nullptr);
	if (settings == nullptr)
		fprintf(stderr, "Out of memory\n");

#ifndef _WIN32
	settings2 = _host == nullptr && _port == 0 &&
		settings_is_local_socket(settings)
		? mpd_settings_new(_host, 6600, _timeout_ms, nullptr, nullptr)
		: nullptr;
#endif

#else
	host = _host;
	port = _port;
#endif
}

static std::string
settings_name(const struct mpd_settings *settings) noexcept
{
	assert(settings != nullptr);

	const char *host = mpd_settings_get_host(settings);
	if (host == nullptr)
		host = "unknown";

	if (host[0] == '/' || host[0] == '@')
		return host;

	unsigned port = mpd_settings_get_port(settings);
	if (port == 0 || port == 6600)
		return host;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%s:%u", host, port);
	return buffer;
}

std::string
mpdclient::GetSettingsName() const noexcept
{
#ifdef ENABLE_ASYNC_CONNECT
	return settings_name(settings);
#else
	struct mpd_settings *settings =
		mpd_settings_new(host, port, 0, nullptr, nullptr);
	if (settings == nullptr)
		return "unknown";

	auto name = settings_name(settings);
	mpd_settings_free(settings);
	return name;
#endif
}

void
mpdclient::WhitelistTags(TagMask mask) noexcept
{
	if (!enable_tag_whitelist) {
		enable_tag_whitelist = true;
		tag_whitelist = TagMask::None();
	}

	tag_whitelist |= mask;
}

void
mpdclient::ClearStatus() noexcept
{
	if (status == nullptr)
		return;

	mpd_status_free(status);
	status = nullptr;

	volume = -1;
	playing = false;
	playing_or_paused = false;
	state = MPD_STATE_UNKNOWN;
}

void
mpdclient::Disconnect() noexcept
{
#ifdef ENABLE_ASYNC_CONNECT
	if (async_connect != nullptr) {
		aconnect_cancel(async_connect);
		async_connect = nullptr;
	}
#endif

	CancelEnterIdle();

	delete source;
	source = nullptr;
	idle = false;
	authenticating = false;

	if (connection) {
		mpd_connection_free(connection);
		++connection_id;
	}
	connection = nullptr;

	ClearStatus();

	playlist.clear();

	current_song = nullptr;

	/* everything has changed after a disconnect */
	events |= MPD_IDLE_ALL;
}

/**
 * Receive a "tagtypes" response and convert it to a #TagMask.
 */
static TagMask
ReceiveTagList(struct mpd_connection *c) noexcept
{
	auto result = TagMask::None();

	while (auto *pair = mpd_recv_tag_type_pair(c)) {
		const auto type = mpd_tag_name_parse(pair->value);
		if (type != MPD_TAG_UNKNOWN)
			result |= type;

		mpd_return_pair(c, pair);
	}

	return result;
}

/**
 * Reset the tag mask and return all tag types known to MPD as a
 * #TagMask.  (Without the reset, we would only see the tags currently
 * enabled for this client.)
 *
 * Returns an empty mask on error.
 */
static TagMask
ResetAndObtainTagList(struct mpd_connection *c) noexcept
{
	if (!mpd_command_list_begin(c, false) ||
#if LIBMPDCLIENT_CHECK_VERSION(2,19,0)
	    !mpd_send_all_tag_types(c) ||
#endif
	    !mpd_send_list_tag_types(c) ||
	    !mpd_command_list_end(c))
		return TagMask::None();

	auto mask = ReceiveTagList(c);
	if (!mpd_response_finish(c))
		return TagMask::None();

	return mask;
}

static bool
SendTagWhitelist(struct mpd_connection *c, const TagMask whitelist) noexcept
{
	const auto available_tags = ResetAndObtainTagList(c);
	if (!available_tags.TestAny())
		return false;

	/* enable only the tags supported by MPD (or else the
	   "tagtypes" request will fail */
	const auto mask = whitelist & available_tags;

	if (!mpd_command_list_begin(c, false) ||
	    !mpd_send_clear_tag_types(c))
		return false;

	/* convert the "tag_bits" mask to an array of enum
	   mpd_tag_type for mpd_send_enable_tag_types() */

	enum mpd_tag_type types[MPD_TAG_COUNT];
	unsigned n = 0;

	for (unsigned i = 0; i < MPD_TAG_COUNT; ++i)
		if (mask.Test((enum mpd_tag_type)i))
			types[n++] = (enum mpd_tag_type)i;

	return (n == 0 || mpd_send_enable_tag_types(c, types, n)) &&
		mpd_command_list_end(c) &&
		mpd_response_finish(c);
}

bool
mpdclient::OnConnected(struct mpd_connection *_connection) noexcept
{
	connection = _connection;

	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		InvokeErrorCallback();
		Disconnect();
		handler.OnMpdConnectFailed();
		return false;
	}

#ifdef ENABLE_ASYNC_CONNECT
	if (timeout_ms > 0)
		mpd_connection_set_timeout(connection, timeout_ms);
#endif

	/* send password */
	if (password != nullptr &&
	    !mpd_run_password(connection, password)) {
		InvokeErrorCallback();
		Disconnect();
		handler.OnMpdConnectFailed();
		return false;
	}

	if (enable_tag_whitelist &&
	    !SendTagWhitelist(connection, tag_whitelist)) {
		InvokeErrorCallback();

		if (!mpd_connection_clear_error(connection)) {
			Disconnect();
			handler.OnMpdConnectFailed();
			return false;
		}
	}

	source = new MpdIdleSource(GetEventLoop(), *connection, *this);
	ScheduleEnterIdle();

	++connection_id;

	/* everything has changed after a connection has been
	   established */
	events = (enum mpd_idle)MPD_IDLE_ALL;

	handler.OnMpdConnected();
	return true;
}

#ifdef ENABLE_ASYNC_CONNECT

void
mpdclient::OnAsyncMpdConnect(struct mpd_connection *_connection) noexcept
{
	assert(async_connect != nullptr);
	async_connect = nullptr;

	const char *password2 =
		mpd_settings_get_password(&GetSettings());
	if (password2 != nullptr && !mpd_run_password(_connection, password2)) {
		handler.OnMpdError(mpd_connection_get_error_message(_connection));
		mpd_connection_free(_connection);
		handler.OnMpdConnectFailed();
		return;
	}

	OnConnected(_connection);
}

void
mpdclient::OnAsyncMpdConnectError(std::exception_ptr e) noexcept
{
	assert(async_connect != nullptr);
	async_connect = nullptr;

#ifndef _WIN32
	if (!connecting2 && settings2 != nullptr) {
		connecting2 = true;
		StartConnect(*settings2);
		return;
	}
#endif

	handler.OnMpdError(std::move(e));
	handler.OnMpdConnectFailed();
}

void
mpdclient::StartConnect(const struct mpd_settings &s) noexcept
{
	aconnect_start(GetEventLoop(), &async_connect,
		       mpd_settings_get_host(&s),
		       mpd_settings_get_port(&s),
		       *this);
}

#endif

void
mpdclient::Connect() noexcept
{
	/* close any open connection */
	Disconnect();

#ifdef ENABLE_ASYNC_CONNECT
#ifndef _WIN32
	connecting2 = false;
#endif
	StartConnect(*settings);
#else
	/* connect to MPD */
	struct mpd_connection *new_connection =
		mpd_connection_new(host, port, timeout_ms);
	if (new_connection == nullptr)
		fprintf(stderr, "Out of memory\n");

	OnConnected(new_connection);
#endif
}

bool
mpdclient::Update() noexcept
{
	auto *c = GetConnection();

	if (c == nullptr)
		return false;

	/* free the old status */
	ClearStatus();

	/* retrieve new status */
	status = mpd_run_status(c);
	if (status == nullptr)
		return HandleError();

	volume = mpd_status_get_volume(status);
	state = mpd_status_get_state(status);
	playing = state == MPD_STATE_PLAY;
	playing_or_paused = state == MPD_STATE_PLAY
		|| state == MPD_STATE_PAUSE;

	/* check if the playlist needs an update */
	if (playlist.version != mpd_status_get_queue_version(status)) {
		bool retval;

		if (!playlist.empty())
			retval = UpdateQueueChanges();
		else
			retval = UpdateQueue();
		if (!retval)
			return false;
	}

	/* update the current song */
	if (current_song == nullptr || mpd_status_get_song_id(status) >= 0)
		current_song = playlist.GetChecked(mpd_status_get_song_pos(status));

	return true;
}

void
mpdclient::AuthenticationFinished() noexcept
{
	assert(IsConnected());

	/* reload everything after a password was submitted */
	events = (enum mpd_idle)MPD_IDLE_ALL;

	ScheduleEnterIdle();

	Update();
}

struct mpd_connection *
mpdclient::GetConnection() noexcept
{
	if (authenticating)
		return nullptr;

	if (source != nullptr && idle) {
		idle = false;
		source->Leave();

		if (source != nullptr)
			ScheduleEnterIdle();
	}

	return connection;
}

const struct mpd_status *
mpdclient::ReceiveStatus() noexcept
{
	assert(connection != nullptr);

	struct mpd_status *new_status = mpd_recv_status(connection);
	if (new_status == nullptr) {
		HandleError();
		return nullptr;
	}

	if (status != nullptr)
		mpd_status_free(status);
	return status = new_status;
}

/****************************************************************************/
/*** MPD Commands  **********************************************************/
/****************************************************************************/

bool
mpdclient_cmd_crop(struct mpdclient &c) noexcept
{
	if (!c.playing_or_paused)
		return false;

	int length = mpd_status_get_queue_length(c.status);
	int current = mpd_status_get_song_pos(c.status);
	if (current < 0 || mpd_status_get_queue_length(c.status) < 2)
		return true;

	struct mpd_connection *connection = c.GetConnection();
	if (connection == nullptr)
		return false;

	mpd_command_list_begin(connection, false);

	if (current < length - 1)
		mpd_send_delete_range(connection, current + 1, length);
	if (current > 0)
		mpd_send_delete_range(connection, 0, current);

	mpd_command_list_end(connection);

	return c.FinishCommand();
}

bool
mpdclient::RunClearQueue() noexcept
{
	auto *c = GetConnection();
	if (c == nullptr)
		return false;

	/* send "clear" and "status" */
	if (!mpd_command_list_begin(c, false) ||
	    !mpd_send_clear(c) ||
	    !mpd_send_status(c) ||
	    !mpd_command_list_end(c))
		return HandleError();

	/* receive the new status, store it in the mpdclient struct */

	const struct mpd_status *new_status = ReceiveStatus();
	if (new_status == nullptr)
		return false;

	if (!mpd_response_finish(c))
		return HandleError();

	/* update mpdclient.playlist */

	if (mpd_status_get_queue_length(new_status) == 0) {
		/* after the "clear" command, the queue is really
		   empty - this means we can clear it locally,
		   reducing the UI latency */
		playlist.clear();
		playlist.version = mpd_status_get_queue_version(new_status);
		current_song = nullptr;
	}

	events |= MPD_IDLE_QUEUE;
	return true;
}

bool
mpdclient::RunVolume(unsigned value) noexcept
{
	return WithConnection([value](struct mpd_connection &c){
		return mpd_run_set_volume(&c, value);
	});
}

bool
mpdclient::RunVolumeUp() noexcept
{
	if (volume < 0 || volume >= 100)
		return true;

	return RunVolume(++volume);
}

bool
mpdclient::RunVolumeDown() noexcept
{
	if (volume <= 0)
		return true;

	return RunVolume(--volume);
}

bool
mpdclient_cmd_add_path(struct mpdclient &c, const char *path_utf8) noexcept
{
	return c.WithConnection([path_utf8](struct mpd_connection &conn){
		return mpd_run_add(&conn, path_utf8);
	});
}

bool
mpdclient::RunAdd(const struct mpd_song &song) noexcept
{
	auto *c = GetConnection();
	if (c == nullptr || status == nullptr)
		return false;

	/* send the add command to mpd; at the same time, get the new
	   status (to verify the new playlist id) and the last song
	   (we hope that's the song we just added) */

	if (!mpd_command_list_begin(c, true) ||
	    !mpd_send_add(c, mpd_song_get_uri(&song)) ||
	    !mpd_send_status(c) ||
	    !mpd_send_get_queue_song_pos(c, playlist.size()) ||
	    !mpd_command_list_end(c) ||
	    !mpd_response_next(c))
		return HandleError();

	events |= MPD_IDLE_QUEUE;

	const struct mpd_status *new_status = ReceiveStatus();
	if (new_status == nullptr)
		return false;

	if (!mpd_response_next(c))
		return HandleError();

	struct mpd_song *new_song = mpd_recv_song(c);
	if (!mpd_response_finish(c) || new_song == nullptr) {
		if (new_song != nullptr)
			mpd_song_free(new_song);

		return mpd_connection_clear_error(c) ||
			HandleError();
	}

	if (mpd_status_get_queue_length(new_status) == playlist.size() + 1 &&
	    mpd_status_get_queue_version(new_status) == playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		playlist.version = mpd_status_get_queue_version(new_status);

		/* the song we just received has the correct id;
		   append it to the local playlist */
		playlist.push_back(*new_song);
	}

	mpd_song_free(new_song);

	return true;
}

bool
mpdclient::RunDelete(unsigned pos) noexcept
{
	auto *c = GetConnection();
	if (c == nullptr || status == nullptr)
		return false;

	if (pos >= playlist.size())
		return false;

	const auto &song = playlist[pos];

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(c, false) ||
	    !mpd_send_delete_id(c, mpd_song_get_id(&song)) ||
	    !mpd_send_status(c) ||
	    !mpd_command_list_end(c))
		return HandleError();

	events |= MPD_IDLE_QUEUE;

	const struct mpd_status *new_status = ReceiveStatus();
	if (new_status == nullptr)
		return false;

	if (!mpd_response_finish(c))
		return HandleError();

	if (mpd_status_get_queue_length(new_status) == playlist.size() - 1 &&
	    mpd_status_get_queue_version(new_status) == playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		playlist.version = mpd_status_get_queue_version(new_status);

		/* remove the song from the local playlist */
		playlist.RemoveIndex(pos);

		/* remove references to the song */
		if (current_song == &song)
			current_song = nullptr;
	}

	return true;
}

bool
mpdclient::RunDeleteRange(unsigned start, unsigned end) noexcept
{
	if (end == start + 1)
		/* if that's not really a range, we choose to use the
		   safer "deleteid" version */
		return RunDelete(start);

	auto *c = GetConnection();
	if (c == nullptr)
		return false;

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(c, false) ||
	    !mpd_send_delete_range(c, start, end) ||
	    !mpd_send_status(c) ||
	    !mpd_command_list_end(c))
		return HandleError();

	events |= MPD_IDLE_QUEUE;

	const struct mpd_status *new_status = ReceiveStatus();
	if (new_status == nullptr)
		return false;

	if (!mpd_response_finish(c))
		return HandleError();

	if (mpd_status_get_queue_length(new_status) == playlist.size() - (end - start) &&
	    mpd_status_get_queue_version(new_status) == playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		playlist.version = mpd_status_get_queue_version(new_status);

		/* remove the song from the local playlist */
		while (end > start) {
			--end;

			/* remove references to the song */
			if (current_song == &playlist[end])
				current_song = nullptr;

			playlist.RemoveIndex(end);
		}
	}

	return true;
}

bool
mpdclient::RunMove(unsigned dest_pos, unsigned src_pos) noexcept
{
	if (dest_pos == src_pos)
		return true;

	auto *c = GetConnection();
	if (c == nullptr)
		return false;

	/* send the "move" command to MPD; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(c, false) ||
	    !mpd_send_move(c, src_pos, dest_pos) ||
	    !mpd_send_status(c) ||
	    !mpd_command_list_end(c))
		return HandleError();

	events |= MPD_IDLE_QUEUE;

	const struct mpd_status *new_status = ReceiveStatus();
	if (status == nullptr)
		return false;

	if (!mpd_response_finish(c))
		return HandleError();

	if (mpd_status_get_queue_length(new_status) == playlist.size() &&
	    mpd_status_get_queue_version(new_status) == playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		playlist.version = mpd_status_get_queue_version(new_status);

		/* swap songs in the local playlist */
		playlist.Move(dest_pos, src_pos);
	}

	return true;
}

/* The client-to-client protocol (MPD 0.17.0) */

bool
mpdclient_cmd_subscribe(struct mpdclient &c, const char *channel) noexcept
{
	return c.WithConnection([channel](struct mpd_connection &conn){
		return mpd_run_subscribe(&conn, channel);
	});
}

bool
mpdclient_cmd_unsubscribe(struct mpdclient &c, const char *channel) noexcept
{
	return c.WithConnection([channel](struct mpd_connection &conn){
		return mpd_run_unsubscribe(&conn, channel);
	});
}

bool
mpdclient_cmd_send_message(struct mpdclient &c, const char *channel,
			   const char *text) noexcept
{
	return c.WithConnection([channel, text](struct mpd_connection &conn){
		return mpd_run_send_message(&conn, channel, text);
	});
}

/****************************************************************************/
/*** Playlist management functions ******************************************/
/****************************************************************************/

/* update playlist */
bool
mpdclient::UpdateQueue() noexcept
{
	auto *c = GetConnection();
	if (c == nullptr)
		return false;

	playlist.clear();

	mpd_send_list_queue_meta(c);

	struct mpd_entity *entity;
	while ((entity = mpd_recv_entity(c))) {
		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
			playlist.push_back(*mpd_entity_get_song(entity));

		mpd_entity_free(entity);
	}

	playlist.version = mpd_status_get_queue_version(status);
	current_song = nullptr;

	return FinishCommand();
}

/* update playlist (plchanges) */
bool
mpdclient::UpdateQueueChanges() noexcept
{
	auto *c = GetConnection();
	if (c == nullptr)
		return false;

	mpd_send_queue_changes_meta(c, playlist.version);

	struct mpd_song *s;
	while ((s = mpd_recv_song(c)) != nullptr) {
		int pos = mpd_song_get_pos(s);

		if (pos >= 0 && (unsigned)pos < playlist.size()) {
			/* update song */
			playlist.Replace(pos, *s);
		} else {
			/* add a new song */
			playlist.push_back(*s);
		}

		mpd_song_free(s);
	}

	/* remove trailing songs */

	unsigned length = mpd_status_get_queue_length(status);
	while (length < playlist.size()) {
		/* Remove the last playlist entry */
		playlist.RemoveIndex(playlist.size() - 1);
	}

	current_song = nullptr;
	playlist.version = mpd_status_get_queue_version(status);

	return FinishCommand();
}
