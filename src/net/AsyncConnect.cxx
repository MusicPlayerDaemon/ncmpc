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

#include "AsyncConnect.hxx"
#include "AsyncHandler.hxx"
#include "util/Compiler.h"

#include <glib.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct AsyncConnect {
	AsyncConnectHandler &handler;

	const socket_t fd;

	guint source_id;

	AsyncConnect(socket_t _fd,
		     AsyncConnectHandler &_handler)
		:handler(_handler),
		 fd(_fd) {}
};

static gboolean
async_connect_source_callback(gcc_unused GIOChannel *source,
			      gcc_unused GIOCondition condition,
			      gpointer data)
{
	auto *ac = (AsyncConnect *)data;

	const int fd = ac->fd;
	auto &handler = ac->handler;
	delete ac;

	int s_err = 0;
	socklen_t s_err_size = sizeof(s_err);

	if (getsockopt(fd, SOL_SOCKET, SO_ERROR,
		       (char*)&s_err, &s_err_size) < 0)
		s_err = -last_socket_error();

	if (s_err == 0) {
		handler.OnConnect(fd);
	} else {
		close_socket(fd);
		char msg[256];
		snprintf(msg, sizeof(msg), "Failed to connect socket: %s",
			 strerror(-s_err));
		handler.OnConnectError(msg);
	}

	return false;
}

void
async_connect_start(AsyncConnect **acp,
		    const struct sockaddr *address, size_t address_size,
		    AsyncConnectHandler &handler)
{
	socket_t fd = create_socket(address->sa_family, SOCK_STREAM, 0);
	if (fd == INVALID_SOCKET) {
		char msg[256];
		snprintf(msg, sizeof(msg), "Failed to create socket: %s",
			 strerror(errno));
		handler.OnConnectError(msg);
		return;
	}

	if (connect(fd, address, address_size) == 0) {
		handler.OnConnect(fd);
		return;
	}

	const int e = last_socket_error();
	if (!would_block(e)) {
		close_socket(fd);
		char msg[256];
		snprintf(msg, sizeof(msg), "Failed to connect socket: %s",
			 strerror(e));
		handler.OnConnectError(msg);
		return;
	}

	AsyncConnect *ac = new AsyncConnect(fd, handler);

	GIOChannel *channel = g_io_channel_unix_new(fd);
	ac->source_id = g_io_add_watch(channel, G_IO_OUT,
				       async_connect_source_callback, ac);
	g_io_channel_unref(channel);

	*acp = ac;
}

void
async_connect_cancel(AsyncConnect *ac)
{
	g_source_remove(ac->source_id);
	close_socket(ac->fd);
	delete ac;
}
