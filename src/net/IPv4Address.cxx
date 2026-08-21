// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "IPv4Address.hxx"

#include <cassert>
#include <cstring> // for strlen()
#include <cstdio> // for sprintf()

#ifndef _WIN32
#include <arpa/inet.h> // for inet_ntop()
#endif

IPv4Address::IPv4Address(SocketAddress src) noexcept
	:address(src.CastTo<struct sockaddr_in>())
{
	assert(!src.IsNull());
	assert(src.GetFamily() == AF_INET);
}

const char *
IPv4Address::Format(std::span<char> buffer) const noexcept
{
	if (buffer.size() <= 7) [[unlikely]]
		return nullptr;

	char *p = buffer.data();

	if (inet_ntop(AF_INET, &address.sin_addr,
		      p, buffer.size() - 7) == nullptr)
		return nullptr;

	const unsigned port = GetPort();
	if (port != 0) {
		p += std::strlen(p);
		std::sprintf(p, ":%u", port);
	}

	return buffer.data();
}
