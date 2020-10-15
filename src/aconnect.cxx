/* ncmpc (Ncurses MPD Client)
   (c) 2004-2020 The Music Player Daemon Project
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
#include "net/AsyncHandler.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "event/SocketEvent.hxx"

#include <mpd/client.h>
#include <mpd/async.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

struct AsyncMpdConnect final : AsyncConnectHandler {
	AsyncMpdConnectHandler &handler;

	AsyncResolveConnect rconnect;

	SocketEvent socket;

	explicit AsyncMpdConnect(EventLoop &event_loop,
				 AsyncMpdConnectHandler &_handler) noexcept
		:handler(_handler),
		 rconnect(event_loop, *this),
		 socket(event_loop, BIND_THIS_METHOD(OnReceive)) {}

	~AsyncMpdConnect() noexcept {
		socket.Close();
	}

	void OnReceive(unsigned events) noexcept;

	/* virtual methods from AsyncConnectHandler */
	void OnConnect(UniqueSocketDescriptor fd) noexcept override;
	void OnConnectError(const char *message) override;
};

void
AsyncMpdConnect::OnReceive(unsigned) noexcept
{
	char buffer[256];
	ssize_t nbytes = socket.GetSocket().Read(buffer, sizeof(buffer));

	if (nbytes < 0) {
		std::snprintf(buffer, sizeof(buffer),
			      "Failed to receive from MPD: %s",
			      std::strerror(errno));
		handler.OnAsyncMpdConnectError(buffer);
		delete this;
		return;
	}

	buffer[nbytes] = 0;

	struct mpd_async *async = mpd_async_new(socket.ReleaseSocket().Get());
	if (async == nullptr) {
		handler.OnAsyncMpdConnectError("Out of memory");
		delete this;
		return;
	}

	struct mpd_connection *c = mpd_connection_new_async(async, buffer);
	if (c == nullptr) {
		mpd_async_free(async);
		handler.OnAsyncMpdConnectError("Out of memory");
		delete this;
		return;
	}

	handler.OnAsyncMpdConnect(c);
	delete this;
}

void
AsyncMpdConnect::OnConnect(UniqueSocketDescriptor fd) noexcept
{
	socket.Open(fd.Release());
	socket.ScheduleRead();
}

void
AsyncMpdConnect::OnConnectError(const char *message)
{
	handler.OnAsyncMpdConnectError(message);
	delete this;
}

void
aconnect_start(EventLoop &event_loop,
	       AsyncMpdConnect **acp,
	       const char *host, unsigned port,
	       AsyncMpdConnectHandler &handler)
{
	auto *ac = new AsyncMpdConnect(event_loop, handler);

	*acp = ac;

	ac->rconnect.Start(host, port);
}

void
aconnect_cancel(AsyncMpdConnect *ac)
{
	delete ac;
}
