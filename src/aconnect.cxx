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

#include <mpd/client.h>
#include <mpd/async.h>

#include <boost/asio/generic/stream_protocol.hpp>

struct AsyncMpdConnect final : AsyncConnectHandler {
	AsyncMpdConnectHandler &handler;

	AsyncResolveConnect rconnect;

	boost::asio::generic::stream_protocol::socket socket;

	char buffer[256];

	explicit AsyncMpdConnect(boost::asio::io_service &io_service,
				 AsyncMpdConnectHandler &_handler) noexcept
		:handler(_handler),
		 rconnect(io_service, *this), socket(io_service) {}

	void OnReceive(const boost::system::error_code &error,
		       std::size_t bytes_transferred) noexcept;

	/* virtual methods from AsyncConnectHandler */
	void OnConnect(boost::asio::generic::stream_protocol::socket socket) override;
	void OnConnectError(const char *message) override;
};

void
AsyncMpdConnect::OnReceive(const boost::system::error_code &error,
			   std::size_t bytes_transferred) noexcept
{
	if (error) {
		if (error == boost::asio::error::operation_aborted)
			/* this object has already been deleted; bail out
			   quickly without touching anything */
			return;

		snprintf(buffer, sizeof(buffer),
			 "Failed to receive from MPD: %s",
			 error.message().c_str());
		handler.OnAsyncMpdConnectError(buffer);
		delete this;
		return;
	}

	buffer[bytes_transferred] = 0;

	/* the dup() is necessary because Boost 1.62 doesn't have the
	   release() method yet */
	struct mpd_async *async = mpd_async_new(dup(socket.native_handle()));
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
AsyncMpdConnect::OnConnect(boost::asio::generic::stream_protocol::socket _socket)
{
	socket = std::move(_socket);
	socket.async_receive(boost::asio::buffer(buffer, sizeof(buffer) - 1),
			     std::bind(&AsyncMpdConnect::OnReceive, this,
				       std::placeholders::_1,
				       std::placeholders::_2));
}

void
AsyncMpdConnect::OnConnectError(const char *message)
{
	handler.OnAsyncMpdConnectError(message);
	delete this;
}

void
aconnect_start(boost::asio::io_service &io_service,
	       AsyncMpdConnect **acp,
	       const char *host, unsigned port,
	       AsyncMpdConnectHandler &handler)
{
	auto *ac = new AsyncMpdConnect(io_service, handler);

	*acp = ac;

	ac->rconnect.Start(io_service, host, port);
}

void
aconnect_cancel(AsyncMpdConnect *ac)
{
	delete ac;
}
