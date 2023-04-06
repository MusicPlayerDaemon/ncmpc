// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#pragma once

#include "event/net/ConnectSocket.hxx"

class AsyncResolveConnect {
	ConnectSocketHandler &handler;

	ConnectSocket connect;

public:
	AsyncResolveConnect(EventLoop &event_loop,
			    ConnectSocketHandler &_handler) noexcept
		:handler(_handler),
		 connect(event_loop, _handler) {}

	/**
	 * Resolve a host name and connect to it asynchronously.
	 */
	void Start(const char *host, unsigned port) noexcept;
};
