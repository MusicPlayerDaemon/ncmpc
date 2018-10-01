/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "mpdclient.hxx"
#include "callbacks.hxx"
#include "filelist.hxx"
#include "config.h"
#include "gidle.hxx"
#include "charset.hxx"

#include <mpd/client.h>

#include <assert.h>

void
mpdclient::OnEnterIdleTimer(const boost::system::error_code &error) noexcept
{
	if (error)
		return;

	assert(source != nullptr);
	assert(!idle);

	idle = source->Enter();
}

void
mpdclient::ScheduleEnterIdle() noexcept
{
	assert(source != nullptr);

	/* automatically re-enter MPD "idle" mode */
	boost::system::error_code error;
	enter_idle_timer.expires_from_now(std::chrono::seconds(0), error);
	enter_idle_timer.async_wait(std::bind(&mpdclient::OnEnterIdleTimer,
					      this, std::placeholders::_1));
}

static void
mpdclient_invoke_error_callback(enum mpd_error error,
				const char *message)
{
	if (error == MPD_ERROR_SERVER)
		/* server errors are UTF-8, the others are locale */
		mpdclient_error_callback(Utf8ToLocale(message).c_str());
	else
		mpdclient_error_callback(message);
}

static void
mpdclient_invoke_error_callback1(struct mpdclient *c)
{
	assert(c != nullptr);
	assert(c->connection != nullptr);

	struct mpd_connection *connection = c->connection;

	enum mpd_error error = mpd_connection_get_error(connection);
	assert(error != MPD_ERROR_SUCCESS);

	mpdclient_invoke_error_callback(error,
					mpd_connection_get_error_message(connection));
}

void
mpdclient::OnIdle(unsigned _events) noexcept
{
	assert(IsConnected());

	idle = false;

	events |= _events;
	Update();

	mpdclient_idle_callback(events);
	events = 0;

	if (source != nullptr)
		ScheduleEnterIdle();
}

void
mpdclient::OnIdleError(enum mpd_error error,
		       gcc_unused enum mpd_server_error server_error,
		       const char *message) noexcept
{
	assert(IsConnected());

	idle = false;

	mpdclient_invoke_error_callback(error, message);
	Disconnect();
	mpdclient_lost_callback();
}

/****************************************************************************/
/*** mpdclient functions ****************************************************/
/****************************************************************************/

bool
mpdclient::HandleError()
{
	enum mpd_error error = mpd_connection_get_error(connection);

	assert(error != MPD_ERROR_SUCCESS);

	if (error == MPD_ERROR_SERVER &&
	    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_PERMISSION &&
	    mpdclient_auth_callback(this))
		return true;

	mpdclient_invoke_error_callback(error,
					mpd_connection_get_error_message(connection));

	if (!mpd_connection_clear_error(connection)) {
		Disconnect();
		mpdclient_lost_callback();
	}

	return false;
}

#ifdef ENABLE_ASYNC_CONNECT
#ifndef _WIN32

static bool
is_local_socket(const char *host)
{
	return *host == '/' || *host == '@';
}

static bool
settings_is_local_socket(const struct mpd_settings *settings)
{
	const char *host = mpd_settings_get_host(settings);
	return host != nullptr && is_local_socket(host);
}

#endif
#endif

mpdclient::mpdclient(boost::asio::io_service &io_service,
		     const char *_host, unsigned _port,
		     unsigned _timeout_ms, const char *_password)
	:timeout_ms(_timeout_ms), password(_password),
	 enter_idle_timer(io_service)
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
settings_name(const struct mpd_settings *settings)
{
	assert(settings != nullptr);

	const char *host = mpd_settings_get_host(settings);
	if (host == nullptr)
		host = "unknown";

	if (host[0] == '/')
		return host;

	unsigned port = mpd_settings_get_port(settings);
	if (port == 0 || port == 6600)
		return host;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%s:%u", host, port);
	return buffer;
}

std::string
mpdclient::GetSettingsName() const
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
	return std::move(name);
#endif
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
mpdclient::Disconnect()
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

bool
mpdclient::OnConnected(struct mpd_connection *_connection) noexcept
{
	connection = _connection;

	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		mpdclient_invoke_error_callback1(this);
		Disconnect();
		mpdclient_failed_callback();
		return false;
	}

#ifdef ENABLE_ASYNC_CONNECT
	if (timeout_ms > 0)
		mpd_connection_set_timeout(connection, timeout_ms);
#endif

	/* send password */
	if (password != nullptr &&
	    !mpd_run_password(connection, password)) {
		mpdclient_invoke_error_callback1(this);
		Disconnect();
		mpdclient_failed_callback();
		return false;
	}

	source = new MpdIdleSource(get_io_service(), *connection, *this);
	ScheduleEnterIdle();

	++connection_id;

	/* everything has changed after a connection has been
	   established */
	events = (enum mpd_idle)MPD_IDLE_ALL;

	mpdclient_connected_callback();
	return true;
}

#ifdef ENABLE_ASYNC_CONNECT

static void
mpdclient_aconnect_start(struct mpdclient *c,
			 const struct mpd_settings *settings);

static const struct mpd_settings *
mpdclient_get_settings(const struct mpdclient *c)
{
#ifndef _WIN32
	if (c->connecting2)
		return c->settings2;
#endif

	return c->settings;
}

void
mpdclient::OnAsyncMpdConnect(struct mpd_connection *_connection) noexcept
{
	assert(async_connect != nullptr);
	async_connect = nullptr;

	const char *password2 =
		mpd_settings_get_password(mpdclient_get_settings(this));
	if (password2 != nullptr && !mpd_run_password(_connection, password2)) {
		mpdclient_error_callback(mpd_connection_get_error_message(_connection));
		mpd_connection_free(_connection);
		mpdclient_failed_callback();
		return;
	}

	OnConnected(_connection);
}

void
mpdclient::OnAsyncMpdConnectError(const char *message) noexcept
{
	assert(async_connect != nullptr);
	async_connect = nullptr;

#ifndef _WIN32
	if (!connecting2 && settings2 != nullptr) {
		connecting2 = true;
		mpdclient_aconnect_start(this, settings2);
		return;
	}
#endif

	mpdclient_error_callback(message);
	mpdclient_failed_callback();
}

static void
mpdclient_aconnect_start(struct mpdclient *c,
			 const struct mpd_settings *settings)
{
	aconnect_start(c->get_io_service(), &c->async_connect,
		       mpd_settings_get_host(settings),
		       mpd_settings_get_port(settings),
		       *c);
}

#endif

void
mpdclient::Connect()
{
	/* close any open connection */
	Disconnect();

#ifdef ENABLE_ASYNC_CONNECT
#ifndef _WIN32
	connecting2 = false;
#endif
	mpdclient_aconnect_start(this, settings);
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
mpdclient::Update()
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

struct mpd_connection *
mpdclient::GetConnection()
{
	if (source != nullptr && idle) {
		idle = false;
		source->Leave();

		if (source != nullptr)
			ScheduleEnterIdle();
	}

	return connection;
}

static struct mpd_status *
mpdclient_recv_status(struct mpdclient *c)
{
	assert(c->connection != nullptr);

	struct mpd_status *status = mpd_recv_status(c->connection);
	if (status == nullptr) {
		c->HandleError();
		return nullptr;
	}

	if (c->status != nullptr)
		mpd_status_free(c->status);
	return c->status = status;
}

/****************************************************************************/
/*** MPD Commands  **********************************************************/
/****************************************************************************/

bool
mpdclient_cmd_crop(struct mpdclient *c)
{
	if (!c->playing_or_paused)
		return false;

	int length = mpd_status_get_queue_length(c->status);
	int current = mpd_status_get_song_pos(c->status);
	if (current < 0 || mpd_status_get_queue_length(c->status) < 2)
		return true;

	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	mpd_command_list_begin(connection, false);

	if (current < length - 1)
		mpd_send_delete_range(connection, current + 1, length);
	if (current > 0)
		mpd_send_delete_range(connection, 0, current);

	mpd_command_list_end(connection);

	return c->FinishCommand();
}

bool
mpdclient_cmd_clear(struct mpdclient *c)
{
	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	/* send "clear" and "status" */
	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_clear(connection) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return c->HandleError();

	/* receive the new status, store it in the mpdclient struct */

	struct mpd_status *status = mpdclient_recv_status(c);
	if (status == nullptr)
		return false;

	if (!mpd_response_finish(connection))
		return c->HandleError();

	/* update mpdclient.playlist */

	if (mpd_status_get_queue_length(status) == 0) {
		/* after the "clear" command, the queue is really
		   empty - this means we can clear it locally,
		   reducing the UI latency */
		c->playlist.clear();
		c->playlist.version = mpd_status_get_queue_version(status);
		c->current_song = nullptr;
	}

	c->events |= MPD_IDLE_QUEUE;
	return true;
}

bool
mpdclient_cmd_volume(struct mpdclient *c, int value)
{
	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	mpd_send_set_volume(connection, value);
	return c->FinishCommand();
}

bool
mpdclient_cmd_volume_up(struct mpdclient *c)
{
	if (c->volume < 0 || c->volume >= 100)
		return true;

	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	return mpdclient_cmd_volume(c, ++c->volume);
}

bool
mpdclient_cmd_volume_down(struct mpdclient *c)
{
	if (c->volume <= 0)
		return true;

	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	return mpdclient_cmd_volume(c, --c->volume);
}

bool
mpdclient_cmd_add_path(struct mpdclient *c, const char *path_utf8)
{
	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	return mpd_send_add(connection, path_utf8)?
		c->FinishCommand() : false;
}

bool
mpdclient_cmd_add(struct mpdclient *c, const struct mpd_song *song)
{
	assert(c != nullptr);
	assert(song != nullptr);

	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr || c->status == nullptr)
		return false;

	/* send the add command to mpd; at the same time, get the new
	   status (to verify the new playlist id) and the last song
	   (we hope that's the song we just added) */

	if (!mpd_command_list_begin(connection, true) ||
	    !mpd_send_add(connection, mpd_song_get_uri(song)) ||
	    !mpd_send_status(connection) ||
	    !mpd_send_get_queue_song_pos(connection,
					 c->playlist.size()) ||
	    !mpd_command_list_end(connection) ||
	    !mpd_response_next(connection))
		return c->HandleError();

	c->events |= MPD_IDLE_QUEUE;

	struct mpd_status *status = mpdclient_recv_status(c);
	if (status == nullptr)
		return false;

	if (!mpd_response_next(connection))
		return c->HandleError();

	struct mpd_song *new_song = mpd_recv_song(connection);
	if (!mpd_response_finish(connection) || new_song == nullptr) {
		if (new_song != nullptr)
			mpd_song_free(new_song);

		return mpd_connection_clear_error(connection) ||
			c->HandleError();
	}

	if (mpd_status_get_queue_length(status) == c->playlist.size() + 1 &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* the song we just received has the correct id;
		   append it to the local playlist */
		c->playlist.push_back(*new_song);
	}

	mpd_song_free(new_song);

	return true;
}

bool
mpdclient_cmd_delete(struct mpdclient *c, int idx)
{
	struct mpd_connection *connection = c->GetConnection();

	if (connection == nullptr || c->status == nullptr)
		return false;

	if (idx < 0 || (unsigned)idx >= c->playlist.size())
		return false;

	const auto &song = c->playlist[idx];

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_delete_id(connection, mpd_song_get_id(&song)) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return c->HandleError();

	c->events |= MPD_IDLE_QUEUE;

	struct mpd_status *status = mpdclient_recv_status(c);
	if (status == nullptr)
		return false;

	if (!mpd_response_finish(connection))
		return c->HandleError();

	if (mpd_status_get_queue_length(status) == c->playlist.size() - 1 &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* remove the song from the local playlist */
		c->playlist.RemoveIndex(idx);

		/* remove references to the song */
		if (c->current_song == &song)
			c->current_song = nullptr;
	}

	return true;
}

bool
mpdclient_cmd_delete_range(struct mpdclient *c, unsigned start, unsigned end)
{
	if (end == start + 1)
		/* if that's not really a range, we choose to use the
		   safer "deleteid" version */
		return mpdclient_cmd_delete(c, start);

	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_delete_range(connection, start, end) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return c->HandleError();

	c->events |= MPD_IDLE_QUEUE;

	struct mpd_status *status = mpdclient_recv_status(c);
	if (status == nullptr)
		return false;

	if (!mpd_response_finish(connection))
		return c->HandleError();

	if (mpd_status_get_queue_length(status) == c->playlist.size() - (end - start) &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* remove the song from the local playlist */
		while (end > start) {
			--end;

			/* remove references to the song */
			if (c->current_song == &c->playlist[end])
				c->current_song = nullptr;

			c->playlist.RemoveIndex(end);
		}
	}

	return true;
}

bool
mpdclient_cmd_move(struct mpdclient *c, unsigned dest_pos, unsigned src_pos)
{
	if (dest_pos == src_pos)
		return true;

	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	/* send the "move" command to MPD; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_move(connection, src_pos, dest_pos) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return c->HandleError();

	c->events |= MPD_IDLE_QUEUE;

	struct mpd_status *status = mpdclient_recv_status(c);
	if (status == nullptr)
		return false;

	if (!mpd_response_finish(connection))
		return c->HandleError();

	if (mpd_status_get_queue_length(status) == c->playlist.size() &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* swap songs in the local playlist */
		c->playlist.Move(dest_pos, src_pos);
	}

	return true;
}

/* The client-to-client protocol (MPD 0.17.0) */

bool
mpdclient_cmd_subscribe(struct mpdclient *c, const char *channel)
{
	struct mpd_connection *connection = c->GetConnection();

	if (connection == nullptr)
		return false;

	if (!mpd_send_subscribe(connection, channel))
		return c->HandleError();

	return c->FinishCommand();
}

bool
mpdclient_cmd_unsubscribe(struct mpdclient *c, const char *channel)
{
	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	if (!mpd_send_unsubscribe(connection, channel))
		return c->HandleError();

	return c->FinishCommand();
}

bool
mpdclient_cmd_send_message(struct mpdclient *c, const char *channel,
			   const char *text)
{
	struct mpd_connection *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	if (!mpd_send_send_message(connection, channel, text))
		return c->HandleError();

	return c->FinishCommand();
}

/****************************************************************************/
/*** Playlist management functions ******************************************/
/****************************************************************************/

/* update playlist */
bool
mpdclient::UpdateQueue()
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
mpdclient::UpdateQueueChanges()
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
