/* ncmpc (Ncurses MPD Client)
   (c) 2004-2018 The Music Player Daemon Project
   Project homepage: http://musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "gidle.hxx"
#include "util/Compiler.h"

#include <mpd/async.h>
#include <mpd/parser.h>

#include <assert.h>
#include <string.h>
#include <errno.h>

MpdIdleSource::MpdIdleSource(boost::asio::io_service &io_service,
			     struct mpd_connection &_connection,
			     mpd_glib_callback_t _callback, void *_callback_ctx)
	:connection(&_connection),
	 async(mpd_connection_get_async(connection)),
	 parser(mpd_parser_new()),
	 callback(_callback), callback_ctx(_callback_ctx),
	 socket(io_service, mpd_async_get_fd(async))
{
	/* TODO check parser!=nullptr */
}

MpdIdleSource::~MpdIdleSource()
{
	socket.release();

	mpd_parser_free(parser);

}

static void
mpd_glib_invoke(const MpdIdleSource *source)
{
	if (source->idle_events != 0)
		source->callback(MPD_ERROR_SUCCESS, (enum mpd_server_error)0,
				 nullptr,
				 source->idle_events, source->callback_ctx);
}

static void
mpd_glib_invoke_error(const MpdIdleSource *source,
		      enum mpd_error error, enum mpd_server_error server_error,
		      const char *message)
{
	source->callback(error, server_error, message,
			 0, source->callback_ctx);
}

static void
mpd_glib_invoke_async_error(const MpdIdleSource *source)
{
	mpd_glib_invoke_error(source, mpd_async_get_error(source->async),
			      (enum mpd_server_error)0,
			      mpd_async_get_error_message(source->async));
}

/**
 * Parses a response line from MPD.
 *
 * @return true on success, false on error
 */
static bool
mpd_glib_feed(MpdIdleSource *source, char *line)
{
	enum mpd_parser_result result;

	result = mpd_parser_feed(source->parser, line);
	switch (result) {
	case MPD_PARSER_MALFORMED:
		source->io_events = 0;

		mpd_glib_invoke_error(source, MPD_ERROR_MALFORMED,
				      (enum mpd_server_error)0,
				      "Malformed MPD response");
		return false;

	case MPD_PARSER_SUCCESS:
		source->io_events = 0;

		mpd_glib_invoke(source);
		return false;

	case MPD_PARSER_ERROR:
		source->io_events = 0;

		mpd_glib_invoke_error(source, MPD_ERROR_SERVER,
				      mpd_parser_get_server_error(source->parser),
				      mpd_parser_get_message(source->parser));
		return false;

	case MPD_PARSER_PAIR:
		if (strcmp(mpd_parser_get_name(source->parser),
			   "changed") == 0)
			source->idle_events |=
				mpd_idle_name_parse(mpd_parser_get_value(source->parser));

		break;
	}

	return true;
}

/**
 * Receives and evaluates a portion of the MPD response.
 *
 * @return true on success, false on error
 */
static bool
mpd_glib_recv(MpdIdleSource *source)
{
	char *line;
	while ((line = mpd_async_recv_line(source->async)) != nullptr) {
		if (!mpd_glib_feed(source, line))
			return false;
	}

	if (mpd_async_get_error(source->async) != MPD_ERROR_SUCCESS) {
		source->io_events = 0;

		mpd_glib_invoke_async_error(source);
		return false;
	}

	return true;
}

void
MpdIdleSource::OnReadable(const boost::system::error_code &error) noexcept
{
	io_events &= ~MPD_ASYNC_EVENT_READ;

	if (error) {
		if (error == boost::asio::error::operation_aborted)
			return;

		// TODO
		return;
	}

	if (!mpd_async_io(async, MPD_ASYNC_EVENT_READ)) {
		socket.cancel();
		io_events = 0;

		mpd_glib_invoke_async_error(this);
		return;
	}

	if (!mpd_glib_recv(this))
		return;

	UpdateSocket();
}

void
MpdIdleSource::OnWritable(const boost::system::error_code &error) noexcept
{
	io_events &= ~MPD_ASYNC_EVENT_WRITE;

	if (error) {
		if (error == boost::asio::error::operation_aborted)
			return;

		// TODO
		return;
	}

	if (!mpd_async_io(async, MPD_ASYNC_EVENT_WRITE)) {
		socket.cancel();
		io_events = 0;

		mpd_glib_invoke_async_error(this);
		return;
	}

	UpdateSocket();
}

void
MpdIdleSource::AsyncRead() noexcept
{
	io_events |= MPD_ASYNC_EVENT_READ;
	socket.async_read_some(boost::asio::null_buffers(),
			       std::bind(&MpdIdleSource::OnReadable, this,
					 std::placeholders::_1));
}

void
MpdIdleSource::AsyncWrite() noexcept
{
	io_events |= MPD_ASYNC_EVENT_WRITE;
	socket.async_write_some(boost::asio::null_buffers(),
				std::bind(&MpdIdleSource::OnWritable, this,
					  std::placeholders::_1));
}

void
MpdIdleSource::UpdateSocket() noexcept
{
	enum mpd_async_event events = mpd_async_events(async);
	if (events == io_events)
		return;

	socket.cancel();

	if (events & MPD_ASYNC_EVENT_READ)
		AsyncRead();

	if (events & MPD_ASYNC_EVENT_WRITE)
		AsyncWrite();

	io_events = events;
}

bool
MpdIdleSource::Enter()
{
	assert(io_events == 0);

	idle_events = 0;

	if (!mpd_async_send_command(async, "idle", nullptr)) {
		mpd_glib_invoke_async_error(this);
		return false;
	}

	UpdateSocket();
	return true;
}

void
MpdIdleSource::Leave()
{
	if (io_events == 0)
		/* already left, callback was invoked */
		return;

	socket.cancel();
	io_events = 0;

	enum mpd_idle events = idle_events == 0
		? mpd_run_noidle(connection)
		: mpd_recv_idle(connection, false);

	if (events == 0 &&
	    mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		enum mpd_error error =
			mpd_connection_get_error(connection);
		enum mpd_server_error server_error =
			error == MPD_ERROR_SERVER
			? mpd_connection_get_server_error(connection)
			: (enum mpd_server_error)0;

		mpd_glib_invoke_error(this, error, server_error,
				      mpd_connection_get_error_message(connection));
		return;
	}

	idle_events |= events;
	mpd_glib_invoke(this);
}
