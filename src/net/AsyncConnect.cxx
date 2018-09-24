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

#include <boost/asio/ip/tcp.hpp>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct AsyncConnect {
	AsyncConnectHandler &handler;

	boost::asio::ip::tcp::socket socket;

	AsyncConnect(boost::asio::io_service &io_service,
		     AsyncConnectHandler &_handler)
		:handler(_handler),
		 socket(io_service) {}

	void OnConnected(const boost::system::error_code &error);
};

void
AsyncConnect::OnConnected(const boost::system::error_code &error)
{
	if (error) {
		if (error == boost::asio::error::operation_aborted)
			/* this object has already been deleted; bail out
			   quickly without touching anything */
			return;

		socket.close();
		handler.OnConnectError(error.message().c_str());
		return;
	}

	handler.OnConnect(std::move(socket));
}

void
async_connect_start(boost::asio::io_service &io_service, AsyncConnect **acp,
		    const boost::asio::ip::tcp::endpoint &endpoint,
		    AsyncConnectHandler &handler)
{
	AsyncConnect *ac = new AsyncConnect(io_service, handler);
	*acp = ac;

	ac->socket.async_connect(endpoint,
				 std::bind(&AsyncConnect::OnConnected, ac,
					   std::placeholders::_1));
}

void
async_connect_cancel(AsyncConnect *ac)
{
	delete ac;
}
