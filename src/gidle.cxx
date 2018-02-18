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
#include "Compiler.h"

#include <mpd/async.h>
#include <mpd/parser.h>

#include <glib.h>

#include <assert.h>
#include <string.h>
#include <errno.h>

struct mpd_glib_source {
	struct mpd_connection *connection;
	struct mpd_async *async;
	struct mpd_parser *parser;

	mpd_glib_callback_t callback;
	void *callback_ctx;

	GIOChannel *channel;

	unsigned io_events;

	guint id;

	unsigned idle_events;

	/**
	 * This flag is a hack: it is set while mpd_glib_leave() is
	 * executed.  mpd_glib_leave() might invoke the callback, and
	 * the callback might invoke mpd_glib_enter(), awkwardly
	 * leaving mpd_glib_leave() in idle mode.  As long as this
	 * flag is set, mpd_glib_enter() is a no-op to prevent this.
	 */
	bool leaving;

	/**
	 * This flag is true when mpd_glib_free() has been called
	 * during a callback invoked from mpd_glib_leave().
	 * mpd_glib_leave() will do the real g_free() call then.
	 */
	bool destroyed;
};

struct mpd_glib_source *
mpd_glib_new(struct mpd_connection *connection,
	     mpd_glib_callback_t callback, void *callback_ctx)
{
	struct mpd_glib_source *source = g_new(struct mpd_glib_source, 1);

	source->connection = connection;
	source->async = mpd_connection_get_async(connection);
	source->parser = mpd_parser_new();
	/* XXX check source->parser!=nullptr */

	source->callback = callback;
	source->callback_ctx = callback_ctx;

	source->channel = g_io_channel_unix_new(mpd_async_get_fd(source->async));
	source->io_events = 0;
	source->id = 0;
	source->leaving = false;
	source->destroyed = false;

	return source;
}

void
mpd_glib_free(struct mpd_glib_source *source)
{
	assert(!source->destroyed);

	if (source->id != 0)
		g_source_remove(source->id);

	g_io_channel_unref(source->channel);

	mpd_parser_free(source->parser);

	if (source->leaving)
		source->destroyed = true;
	else
		g_free(source);
}

static void
mpd_glib_invoke(const struct mpd_glib_source *source)
{
	assert(source->id == 0);
	assert(!source->destroyed);

	if (source->idle_events != 0)
		source->callback(MPD_ERROR_SUCCESS, (enum mpd_server_error)0,
				 nullptr,
				 source->idle_events, source->callback_ctx);
}

static void
mpd_glib_invoke_error(const struct mpd_glib_source *source,
		      enum mpd_error error, enum mpd_server_error server_error,
		      const char *message)
{
	assert(source->id == 0);
	assert(!source->destroyed);

	source->callback(error, server_error, message,
			 0, source->callback_ctx);
}

static void
mpd_glib_invoke_async_error(const struct mpd_glib_source *source)
{
	assert(source->id == 0);

	mpd_glib_invoke_error(source, mpd_async_get_error(source->async),
			      (enum mpd_server_error)0,
			      mpd_async_get_error_message(source->async));
}

/**
 * Converts a GIOCondition bit mask to #mpd_async_event.
 */
static enum mpd_async_event
g_io_condition_to_mpd_async_event(GIOCondition condition)
{
	unsigned events = 0;

	if (condition & G_IO_IN)
		events |= MPD_ASYNC_EVENT_READ;

	if (condition & G_IO_OUT)
		events |= MPD_ASYNC_EVENT_WRITE;

	if (condition & G_IO_HUP)
		events |= MPD_ASYNC_EVENT_HUP;

	if (condition & G_IO_ERR)
		events |= MPD_ASYNC_EVENT_ERROR;

	return (enum mpd_async_event)events;
}

/**
 * Converts a #mpd_async_event bit mask to GIOCondition.
 */
static GIOCondition
mpd_async_events_to_g_io_condition(enum mpd_async_event events)
{
	unsigned condition = 0;

	if (events & MPD_ASYNC_EVENT_READ)
		condition |= G_IO_IN;

	if (events & MPD_ASYNC_EVENT_WRITE)
		condition |= G_IO_OUT;

	if (events & MPD_ASYNC_EVENT_HUP)
		condition |= G_IO_HUP;

	if (events & MPD_ASYNC_EVENT_ERROR)
		condition |= G_IO_ERR;

	return GIOCondition(condition);
}

/**
 * Parses a response line from MPD.
 *
 * @return true on success, false on error
 */
static bool
mpd_glib_feed(struct mpd_glib_source *source, char *line)
{
	enum mpd_parser_result result;

	result = mpd_parser_feed(source->parser, line);
	switch (result) {
	case MPD_PARSER_MALFORMED:
		source->id = 0;
		source->io_events = 0;

		mpd_glib_invoke_error(source, MPD_ERROR_MALFORMED,
				      (enum mpd_server_error)0,
				      "Malformed MPD response");
		return false;

	case MPD_PARSER_SUCCESS:
		source->id = 0;
		source->io_events = 0;

		mpd_glib_invoke(source);
		return false;

	case MPD_PARSER_ERROR:
		source->id = 0;
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
mpd_glib_recv(struct mpd_glib_source *source)
{
	char *line;
	while ((line = mpd_async_recv_line(source->async)) != nullptr) {
		if (!mpd_glib_feed(source, line))
			return false;
	}

	if (mpd_async_get_error(source->async) != MPD_ERROR_SUCCESS) {
		source->id = 0;
		source->io_events = 0;

		mpd_glib_invoke_async_error(source);
		return false;
	}

	return true;
}

static gboolean
mpd_glib_source_callback(gcc_unused GIOChannel *_source,
			 GIOCondition condition, gpointer data)
{
	auto *source = (struct mpd_glib_source *)data;

	assert(source->id != 0);
	assert(source->io_events != 0);

	/* let libmpdclient do some I/O */

	if (!mpd_async_io(source->async,
			  g_io_condition_to_mpd_async_event(condition))) {
		source->id = 0;
		source->io_events = 0;

		mpd_glib_invoke_async_error(source);
		return false;
	}

	/* receive the response */

	if ((condition & G_IO_IN) != 0) {
		if (!mpd_glib_recv(source))
			return false;
	}

	/* continue polling? */

	enum mpd_async_event events = mpd_async_events(source->async);
	if (events == 0) {
		/* no events - disable watch */
		source->id = 0;
		source->io_events = 0;

		return false;
	} else if (events != source->io_events) {
		/* different event mask: make new watch */

		g_source_remove(source->id);

		condition = mpd_async_events_to_g_io_condition(events);
		source->id = g_io_add_watch(source->channel, condition,
					    mpd_glib_source_callback, source);
		source->io_events = events;

		return false;
	} else
		/* same event mask as before, enable the old watch */
		return true;
}

static void
mpd_glib_add_watch(struct mpd_glib_source *source)
{
	enum mpd_async_event events = mpd_async_events(source->async);

	assert(source->io_events == 0);
	assert(source->id == 0);

	GIOCondition condition = mpd_async_events_to_g_io_condition(events);
	source->id = g_io_add_watch(source->channel, condition,
				    mpd_glib_source_callback, source);
	source->io_events = events;
}

bool
mpd_glib_enter(struct mpd_glib_source *source)
{
	assert(source->io_events == 0);
	assert(source->id == 0);
	assert(!source->destroyed);

	if (source->leaving)
		return false;

	source->idle_events = 0;

	if (!mpd_async_send_command(source->async, "idle", nullptr)) {
		mpd_glib_invoke_async_error(source);
		return false;
	}

	mpd_glib_add_watch(source);
	return true;
}

bool
mpd_glib_leave(struct mpd_glib_source *source)
{
	assert(!source->destroyed);

	if (source->id == 0)
		/* already left, callback was invoked */
		return true;

	g_source_remove(source->id);
	source->id = 0;
	source->io_events = 0;

	enum mpd_idle events = source->idle_events == 0
		? mpd_run_noidle(source->connection)
		: mpd_recv_idle(source->connection, false);

	source->leaving = true;

	if (events == 0 &&
	    mpd_connection_get_error(source->connection) != MPD_ERROR_SUCCESS) {
		enum mpd_error error =
			mpd_connection_get_error(source->connection);
		enum mpd_server_error server_error =
			error == MPD_ERROR_SERVER
			? mpd_connection_get_server_error(source->connection)
			: (enum mpd_server_error)0;

		mpd_glib_invoke_error(source, error, server_error,
				      mpd_connection_get_error_message(source->connection));

		if (source->destroyed) {
			g_free(source);
			return false;
		}

		source->leaving = false;
		return true;
	}

	source->idle_events |= events;
	mpd_glib_invoke(source);

	if (source->destroyed) {
		g_free(source);
		return false;
	}

	source->leaving = false;
	return true;
}
