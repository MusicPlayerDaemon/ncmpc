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

#include "AsyncResolveConnect.hxx"
#include "AsyncConnect.hxx"
#include "AsyncHandler.hxx"
#include "AllocatedSocketAddress.hxx"
#include "AddressInfo.hxx"
#include "Resolver.hxx"

#include <exception>

void
AsyncResolveConnect::Start(const char *host, unsigned port) noexcept
{
#ifndef _WIN32
	if (host[0] == '/' || host[0] == '@') {
		AllocatedSocketAddress address;
		address.SetLocal(host);

		connect.Start(address);
		return;
	}
#endif /* _WIN32 */

	try {
		connect.Start(Resolve(host, port, 0, SOCK_STREAM).GetBest());
	} catch (const std::exception &e) {
		handler.OnConnectError(e.what());
	}
}
