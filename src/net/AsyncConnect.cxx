/* ncmpc (Ncurses MPD Client)
   Copyright 2004-2021 The Music Player Daemon Project

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
#include "net/UniqueSocketDescriptor.hxx"
#include "net/SocketAddress.hxx"
#include "system/Error.hxx"

#include <cassert>
#include <exception>

static UniqueSocketDescriptor
Connect(const SocketAddress address)
{
	UniqueSocketDescriptor fd;
	if (!fd.CreateNonBlock(address.GetFamily(), SOCK_STREAM, 0))
		throw MakeErrno("Failed to create socket");

	if (!fd.Connect(address) && errno != EINPROGRESS)
		throw MakeErrno("Failed to connect");

	return fd;
}

bool
AsyncConnect::Start(const SocketAddress address) noexcept
{
	assert(!event.IsDefined());

	try {
		WaitConnected(::Connect(address));
		return true;
	} catch (...) {
		handler.OnConnectError(std::current_exception());
		return false;
	}
}

void
AsyncConnect::WaitConnected(UniqueSocketDescriptor fd) noexcept
{
	assert(!event.IsDefined());

	event.Open(fd.Release());
	event.ScheduleWrite();
}

void
AsyncConnect::OnSocketReady(unsigned events) noexcept
{
	if (SocketEvent::ERROR == 0 || events & SocketEvent::ERROR) {
		int s_err = event.GetSocket().GetError();
		if (s_err != 0) {
			event.Close();
			handler.OnConnectError(std::make_exception_ptr(MakeErrno(s_err, "Failed to connect")));
			return;
		}
	}

	handler.OnConnect(UniqueSocketDescriptor(event.ReleaseSocket()));
}
