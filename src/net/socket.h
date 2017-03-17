/* ncmpc (Ncurses MPD Client)
   (c) 2004-2017 The Music Player Daemon Project
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

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "types.h"

#include <stdbool.h>

#ifndef WIN32
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#endif

socket_t
create_socket(int domain, int type, int protocol);

static inline void
close_socket(socket_t s)
{
#ifdef WIN32
	closesocket(s);
#else
	close(s);
#endif
}

static inline int
last_socket_error(void)
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

static inline bool
would_block(int e)
{
#ifdef WIN32
	return e == WSAEINPROGRESS || e == WSAEWOULDBLOCK;
#else
	return e == EINPROGRESS || e == EAGAIN;
#endif
}

#endif
