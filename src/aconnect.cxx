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

#include "aconnect.hxx"
#include "net/AsyncResolveConnect.hxx"
#include "net/socket.hxx"
#include "util/Compiler.h"

#include <mpd/client.h>
#include <mpd/async.h>

#include <glib.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

struct AsyncMpdConnect {
	const AsyncMpdConnectHandler *handler;
	void *handler_ctx;

	AsyncResolveConnect *rconnect;

	int fd;
	guint source_id;
};

static gboolean
aconnect_source_callback(gcc_unused GIOChannel *source,
			 gcc_unused GIOCondition condition,
			 gpointer data)
{
	auto *ac = (AsyncMpdConnect *)data;
	assert(ac->source_id != 0);
	ac->source_id = 0;

	char buffer[256];
	ssize_t nbytes = recv(ac->fd, buffer, sizeof(buffer) - 1, 0);
	if (nbytes < 0) {
		snprintf(buffer, sizeof(buffer),
			 "Failed to receive from MPD: %s",
			 strerror(errno));
		close_socket(ac->fd);
		ac->handler->error(buffer, ac->handler_ctx);
		delete ac;
		return false;
	}

	if (nbytes == 0) {
		close_socket(ac->fd);
		ac->handler->error("MPD closed the connection",
				   ac->handler_ctx);
		delete ac;
		return false;
	}

	buffer[nbytes] = 0;

	struct mpd_async *async = mpd_async_new(ac->fd);
	if (async == nullptr) {
		close_socket(ac->fd);
		ac->handler->error("Out of memory", ac->handler_ctx);
		delete ac;
		return false;
	}

	struct mpd_connection *c = mpd_connection_new_async(async, buffer);
	if (c == nullptr) {
		mpd_async_free(async);
		ac->handler->error("Out of memory", ac->handler_ctx);
		delete ac;
		return false;
	}

	ac->handler->success(c, ac->handler_ctx);
	delete ac;
	return false;
}

static void
aconnect_rconnect_success(int fd, void *ctx)
{
	auto *ac = (AsyncMpdConnect *)ctx;
	ac->rconnect = nullptr;

	ac->fd = fd;

	GIOChannel *channel = g_io_channel_unix_new(fd);
	ac->source_id = g_io_add_watch(channel, G_IO_IN,
				       aconnect_source_callback, ac);
	g_io_channel_unref(channel);
}

static void
aconnect_rconnect_error(const char *message, void *ctx)
{
	auto *ac = (AsyncMpdConnect *)ctx;
	ac->rconnect = nullptr;

	ac->handler->error(message, ac->handler_ctx);
	delete ac;
}

static const AsyncResolveConnectHandler aconnect_rconnect_handler = {
	.success = aconnect_rconnect_success,
	.error = aconnect_rconnect_error,
};

void
aconnect_start(AsyncMpdConnect **acp,
	       const char *host, unsigned port,
	       const AsyncMpdConnectHandler &handler, void *ctx)
{
	auto *ac = new AsyncMpdConnect();
	ac->handler = &handler;
	ac->handler_ctx = ctx;

	*acp = ac;

	async_rconnect_start(&ac->rconnect, host, port,
			     &aconnect_rconnect_handler, ac);
}

void
aconnect_cancel(AsyncMpdConnect *ac)
{
	if (ac->rconnect != nullptr)
		async_rconnect_cancel(ac->rconnect);
	else {
		g_source_remove(ac->source_id);
		close_socket(ac->fd);
	}

	delete ac;
}
