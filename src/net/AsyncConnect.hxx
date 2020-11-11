/* ncmpc (Ncurses MPD Client)
   (c) 2004-2020 The Music Player Daemon Project

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

#ifndef NET_ASYNC_CONNECT_HXX
#define NET_ASYNC_CONNECT_HXX

#include "event/SocketEvent.hxx"

class SocketAddress;
class AsyncConnectHandler;
class UniqueSocketDescriptor;

class AsyncConnect {
	SocketEvent event;

	AsyncConnectHandler &handler;

public:
	AsyncConnect(EventLoop &event_loop,
		     AsyncConnectHandler &_handler) noexcept
		:event(event_loop, BIND_THIS_METHOD(OnSocketReady)),
		 handler(_handler) {}

	~AsyncConnect() noexcept {
		event.Close();
	}

	/**
	 * Create a socket and connect it to the given address.
	 */
	bool Start(SocketAddress address) noexcept;

	void WaitConnected(UniqueSocketDescriptor fd) noexcept;

private:
	void OnSocketReady(unsigned events) noexcept;
};

#endif
