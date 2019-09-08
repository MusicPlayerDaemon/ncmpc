/* ncmpc (Ncurses MPD Client)
   (c) 2004-2019 The Music Player Daemon Project
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

void
AsyncConnect::OnConnected(const boost::system::error_code &error) noexcept
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
AsyncConnect::Start(const boost::asio::ip::tcp::endpoint &endpoint) noexcept
{
	socket.async_connect(endpoint,
			     std::bind(&AsyncConnect::OnConnected, this,
				       std::placeholders::_1));
}
