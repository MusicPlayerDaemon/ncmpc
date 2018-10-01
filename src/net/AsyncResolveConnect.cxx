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

#ifndef _WIN32
#include <boost/asio/local/stream_protocol.hpp>
#endif

#include <string>

void
AsyncResolveConnect::OnResolved(const boost::system::error_code &error,
				boost::asio::ip::tcp::resolver::iterator i) noexcept
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
AsyncResolveConnect::Start(const char *host, unsigned port) noexcept
{
#ifndef _WIN32
	if (host[0] == '/' || host[0] == '@') {
		std::string s(host);
		if (host[0] == '@')
			/* abstract socket */
			s.front() = 0;

		boost::asio::local::stream_protocol::endpoint ep(std::move(s));
		boost::asio::local::stream_protocol::socket socket(resolver.get_io_service());

		boost::system::error_code error;
		socket.connect(ep, error);
		if (error) {
			handler.OnConnectError(error.message().c_str());
			return;
		}

		handler.OnConnect(std::move(socket));
		return;
	}
#endif /* _WIN32 */

	char service[20];
	snprintf(service, sizeof(service), "%u", port);

	resolver.async_resolve({host, service},
			       std::bind(&AsyncResolveConnect::OnResolved,
					 this,
					 std::placeholders::_1,
					 std::placeholders::_2));
}
