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

#include "AsyncResolveConnect.hxx"
#include "AsyncConnect.hxx"
#include "AsyncHandler.hxx"
#include "util/Compiler.h"

#ifndef _WIN32
#include <boost/asio/local/stream_protocol.hpp>
#endif

#include <string>

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct AsyncResolveConnect final : AsyncConnectHandler {
	AsyncConnectHandler &handler;

	boost::asio::ip::tcp::resolver resolver;

	AsyncConnect connect;

	AsyncResolveConnect(boost::asio::io_service &io_service,
			    AsyncConnectHandler &_handler)
		:handler(_handler), resolver(io_service),
		 connect(io_service, *this) {}

	void OnResolved(const boost::system::error_code &error,
			boost::asio::ip::tcp::resolver::iterator i);

	/* virtual methods from AsyncConnectHandler */
	void OnConnect(boost::asio::generic::stream_protocol::socket socket) override;
	void OnConnectError(const char *message) override;
};

void
AsyncResolveConnect::OnConnect(boost::asio::generic::stream_protocol::socket socket)
{
	handler.OnConnect(std::move(socket));

	delete this;
}

void
AsyncResolveConnect::OnConnectError(const char *message)
{
	handler.OnConnectError(message);
}

void
AsyncResolveConnect::OnResolved(const boost::system::error_code &error,
				boost::asio::ip::tcp::resolver::iterator i)
{
	if (error) {
		if (error == boost::asio::error::operation_aborted)
			/* this object has already been deleted; bail
			   out quickly without touching anything */
			return;

		handler.OnConnectError(error.message().c_str());
		return;
	}

	connect.Start(*i);
}

void
async_rconnect_start(boost::asio::io_service &io_service,
		     AsyncResolveConnect **rcp,
		     const char *host, unsigned port,
		     AsyncConnectHandler &handler)
{
#ifndef _WIN32
	if (host[0] == '/' || host[0] == '@') {
		*rcp = nullptr;

		std::string s(host);
		if (host[0] == '@')
			/* abstract socket */
			s.front() = 0;

		boost::asio::local::stream_protocol::endpoint ep(std::move(s));
		boost::asio::local::stream_protocol::socket socket(io_service);
		socket.connect(ep);

		handler.OnConnect(std::move(socket));
		return;
	}
#endif /* _WIN32 */

	char service[20];
	snprintf(service, sizeof(service), "%u", port);

	auto *rc = new AsyncResolveConnect(io_service, handler);
	*rcp = rc;

	rc->resolver.async_resolve({host, service},
				   std::bind(&AsyncResolveConnect::OnResolved,
					     rc,
					     std::placeholders::_1,
					     std::placeholders::_2));
}

void
async_rconnect_cancel(AsyncResolveConnect *rc)
{
	delete rc;
}
