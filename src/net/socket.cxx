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

#include "socket.hxx"

#ifndef _WIN32
#include <fcntl.h>
#endif

static void
socket_set_cloexec(socket_t fd)
{
#ifndef _WIN32
	fcntl(fd, F_SETFD, FD_CLOEXEC);
#else
	(void)fd;
#endif
}

static void
socket_set_nonblock(socket_t fd)
{
#ifdef _WIN32
	u_long val = 1;
	ioctlsocket(fd, FIONBIO, &val);
#else
	int flags = fcntl(fd, F_GETFL);
	if (flags >= 0)
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

socket_t
create_socket(int domain, int type, int protocol)
{
	socket_t fd;

#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
	fd = socket(domain, type | SOCK_CLOEXEC | SOCK_NONBLOCK, protocol);
	if (fd != INVALID_SOCKET || errno != EINVAL)
		return fd;
#endif

	fd = socket(domain, type, protocol);
	if (fd != INVALID_SOCKET) {
		socket_set_cloexec(fd);
		socket_set_nonblock(fd);
	}

	return fd;
}
