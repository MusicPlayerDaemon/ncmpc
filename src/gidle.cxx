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

void
MpdIdleSource::InvokeAsyncError() noexcept
{
	InvokeError(mpd_async_get_error(async),
		    (enum mpd_server_error)0,
		    mpd_async_get_error_message(async));
}

bool
MpdIdleSource::Feed(char *line) noexcept
{
	enum mpd_parser_result result;

	result = mpd_parser_feed(parser, line);
	switch (result) {
	case MPD_PARSER_MALFORMED:
		io_events = 0;

		InvokeError(MPD_ERROR_MALFORMED,
			    (enum mpd_server_error)0,
			    "Malformed MPD response");
		return false;

	case MPD_PARSER_SUCCESS:
		io_events = 0;

		InvokeCallback();
		return false;

	case MPD_PARSER_ERROR:
		io_events = 0;

		InvokeError(MPD_ERROR_SERVER,
			    mpd_parser_get_server_error(parser),
			    mpd_parser_get_message(parser));
		return false;

	case MPD_PARSER_PAIR:
		if (strcmp(mpd_parser_get_name(parser),
			   "changed") == 0)
			idle_events |=
				mpd_idle_name_parse(mpd_parser_get_value(parser));

		break;
	}

	return true;
}

bool
MpdIdleSource::Receive() noexcept
{
	char *line;
	while ((line = mpd_async_recv_line(async)) != nullptr) {
		if (!Feed(line))
			return false;
	}

	if (mpd_async_get_error(async) != MPD_ERROR_SUCCESS) {
		io_events = 0;

		InvokeAsyncError();
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

		InvokeAsyncError();
		return;
	}

	if (!Receive())
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

		InvokeAsyncError();
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
		InvokeAsyncError();
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

		InvokeError(error, server_error,
			    mpd_connection_get_error_message(connection));
		return;
	}

	idle_events |= events;
	InvokeCallback();
}
