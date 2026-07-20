// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "aconnect.hxx"
#include "net/AsyncResolveConnect.hxx"
#include "net/SocketError.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "system/Error.hxx"
#include "event/SocketEvent.hxx"

#include <mpd/client.h>
#include <mpd/async.h>

#include <cstdio>
#include <cstring>

struct AsyncMpdConnect final : ConnectSocketHandler {
	AsyncMpdConnectHandler &handler;

	AsyncResolveConnect rconnect;

	SocketEvent socket;

	explicit AsyncMpdConnect(EventLoop &event_loop,
				 AsyncMpdConnectHandler &_handler) noexcept
		:handler(_handler),
		 rconnect(event_loop, *this),
		 socket(event_loop, BIND_THIS_METHOD(OnReceive)) {}

	~AsyncMpdConnect() noexcept {
		socket.Close();
	}

	void OnReceive(unsigned events) noexcept;

	/* virtual methods from AsyncConnectHandler */
	void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept override;
	void OnSocketConnectError(std::exception_ptr ep) noexcept override;
};

/**
 * Receive the "welcome" line from MPD and return it as a
 * null-terminated string.
 *
 * Throw on error.
 */
static const char *
ReceiveWelcome(SocketDescriptor socket, std::span<char> buffer)
{
	ssize_t nbytes = socket.ReadNoWait(std::as_writable_bytes(buffer));

	if (nbytes < 0)
		throw MakeSocketError("Failed to receive from MPD");

	// null-terminate it
	buffer[nbytes] = '\0';

	return buffer.data();
}

void
AsyncMpdConnect::OnReceive(unsigned) noexcept
try {
	char buffer[256];

	const char *welcome = ReceiveWelcome(socket.GetSocket(), buffer);

	struct mpd_async *async = mpd_async_new(socket.ReleaseSocket().Get());
	if (async == nullptr)
		throw std::bad_alloc{};

	struct mpd_connection *c = mpd_connection_new_async(async, welcome);
	if (c == nullptr) {
		mpd_async_free(async);
		throw std::bad_alloc{};
	}

	handler.OnAsyncMpdConnect(c);
	delete this;
} catch (...) {
	handler.OnAsyncMpdConnectError(std::current_exception());
	delete this;
}

void
AsyncMpdConnect::OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept
{
	socket.Open(fd.Release());
	socket.ScheduleRead();
}

void
AsyncMpdConnect::OnSocketConnectError(std::exception_ptr e) noexcept
{
	handler.OnAsyncMpdConnectError(std::move(e));
	delete this;
}

void
aconnect_start(EventLoop &event_loop,
	       AsyncMpdConnect **acp,
	       const char *host, unsigned port,
	       AsyncMpdConnectHandler &handler)
{
	auto *ac = new AsyncMpdConnect(event_loop, handler);

	*acp = ac;

	ac->rconnect.Start(host, port);
}

void
aconnect_cancel(AsyncMpdConnect *ac)
{
	delete ac;
}
