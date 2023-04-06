// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "gidle.hxx"

#include <mpd/async.h>
#include <mpd/parser.h>

#include <assert.h>
#include <string.h>

MpdIdleSource::MpdIdleSource(EventLoop &event_loop,
			     struct mpd_connection &_connection,
			     MpdIdleHandler &_handler) noexcept
	:connection(&_connection),
	 async(mpd_connection_get_async(connection)),
	 parser(mpd_parser_new()),
	 event(event_loop, BIND_THIS_METHOD(OnSocketReady),
	       SocketDescriptor(mpd_async_get_fd(async))),
	 handler(_handler)
{
	/* TODO check parser!=nullptr */
}

MpdIdleSource::~MpdIdleSource() noexcept
{
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
		event.Cancel();
		io_events = 0;

		InvokeError(MPD_ERROR_MALFORMED,
			    (enum mpd_server_error)0,
			    "Malformed MPD response");
		return false;

	case MPD_PARSER_SUCCESS:
		event.Cancel();
		io_events = 0;

		InvokeCallback();
		return false;

	case MPD_PARSER_ERROR:
		event.Cancel();
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
		event.Cancel();
		io_events = 0;

		InvokeAsyncError();
		return false;
	}

	return true;
}

void
MpdIdleSource::OnSocketReady(unsigned flags) noexcept
{
	unsigned events = 0;
	if (flags & SocketEvent::READ)
		events |= MPD_ASYNC_EVENT_READ;
	if (flags & SocketEvent::WRITE)
		events |= MPD_ASYNC_EVENT_WRITE;

	if (!mpd_async_io(async, (enum mpd_async_event)events)) {
		event.Cancel();
		io_events = 0;

		InvokeAsyncError();
		return;
	}

	if (flags & SocketEvent::READ)
		if (!Receive())
			return;

	UpdateSocket();
}

void
MpdIdleSource::UpdateSocket() noexcept
{
	enum mpd_async_event events = mpd_async_events(async);
	if (events == io_events)
		return;

	unsigned flags = 0;
	if (events & MPD_ASYNC_EVENT_READ)
		flags |= SocketEvent::READ;

	if (events & MPD_ASYNC_EVENT_WRITE)
		flags |= SocketEvent::WRITE;

	event.Schedule(flags);

	io_events = events;
}

bool
MpdIdleSource::Enter() noexcept
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
MpdIdleSource::Leave() noexcept
{
	if (io_events == 0)
		/* already left, callback was invoked */
		return;

	event.Cancel();
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
