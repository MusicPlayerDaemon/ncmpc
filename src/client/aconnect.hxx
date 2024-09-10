// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#pragma once

#include <exception>

#include <mpd/client.h>

struct mpd_connection;
struct AsyncMpdConnect;
class EventLoop;

class AsyncMpdConnectHandler {
public:
	virtual void OnAsyncMpdConnect(struct mpd_connection *c) noexcept = 0;
	virtual void OnAsyncMpdConnectError(std::exception_ptr e) noexcept = 0;
};

void
aconnect_start(EventLoop &event_loop,
	       AsyncMpdConnect **acp,
	       const char *host, unsigned port,
	       AsyncMpdConnectHandler &handler);

void
aconnect_cancel(AsyncMpdConnect *ac);
