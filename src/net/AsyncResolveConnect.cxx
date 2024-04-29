// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "AsyncResolveConnect.hxx"
#include "LocalSocketAddress.hxx"
#include "AddressInfo.hxx"
#include "Resolver.hxx"

#include <exception>

void
AsyncResolveConnect::Start(const char *host, unsigned port) noexcept
{
#ifndef _WIN32
	if (host[0] == '/' || host[0] == '@') {
		connect.Connect(LocalSocketAddress{host},
				std::chrono::seconds(30));
		return;
	}
#endif /* _WIN32 */

	try {
		connect.Connect(Resolve(host, port, AI_ADDRCONFIG, SOCK_STREAM).GetBest(),
				std::chrono::seconds(30));
	} catch (...) {
		handler.OnSocketConnectError(std::current_exception());
	}
}
