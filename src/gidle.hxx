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

#ifndef MPD_GLIB_SOURCE_H
#define MPD_GLIB_SOURCE_H

#include <mpd/client.h>

#include <glib.h>

typedef void (*mpd_glib_callback_t)(enum mpd_error error,
				    enum mpd_server_error server_error,
				    const char *message,
				    unsigned events, void *ctx);

struct MpdIdleSource {
	struct mpd_connection *connection;
	struct mpd_async *async;
	struct mpd_parser *parser;

	mpd_glib_callback_t callback;
	void *callback_ctx;

	GIOChannel *channel;

	unsigned io_events = 0;

	guint id = 0;

	unsigned idle_events;

	MpdIdleSource(struct mpd_connection &_connection,
		      mpd_glib_callback_t _callback, void *_callback_ctx);
};

MpdIdleSource *
mpd_glib_new(struct mpd_connection *connection,
	     mpd_glib_callback_t callback, void *callback_ctx);

void
mpd_glib_free(MpdIdleSource *source);

/**
 * Enters idle mode.
 *
 * @return true if idle mode has been entered, false if not
 * (e.g. blocked during the callback, or I/O error)
 */
bool
mpd_glib_enter(MpdIdleSource *source);

/**
 * Leaves idle mode and invokes the callback if there were events.
 */
void
mpd_glib_leave(MpdIdleSource *source);

#endif
