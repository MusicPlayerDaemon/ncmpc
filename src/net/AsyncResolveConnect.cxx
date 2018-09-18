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

#include "AsyncResolveConnect.hxx"
#include "AsyncConnect.hxx"
#include "resolver.hxx"
#include "util/Compiler.h"

#include <string>

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct AsyncResolveConnect final : AsyncConnectHandler {
	const AsyncResolveConnectHandler *handler;
	void *handler_ctx;

	const char *host;
	struct resolver *resolver;

	AsyncConnect *connect = nullptr;

	std::string last_error;

	AsyncResolveConnect(const char *_host, struct resolver *_resolver,
			    const AsyncResolveConnectHandler &_handler,
			    void *_ctx)
		:handler(&_handler), handler_ctx(_ctx),
		 host(_host), resolver(_resolver) {}

	~AsyncResolveConnect() {
		if (connect != nullptr)
			async_connect_cancel(connect);
		resolver_free(resolver);
	}

	/* virtual methods from AsyncConnectHandler */
	void OnConnect(socket_t fd) override;
	void OnConnectError(const char *message) override;
};

static void
async_rconnect_next(AsyncResolveConnect *rc);

void
AsyncResolveConnect::OnConnect(socket_t fd)
{
	connect = nullptr;

	handler->success(fd, handler_ctx);

	delete this;
}

void
AsyncResolveConnect::OnConnectError(const char *message)
{
	connect = nullptr;

	last_error = message;

	async_rconnect_next(this);
}

static void
async_rconnect_next(AsyncResolveConnect *rc)
{
	assert(rc->connect == nullptr);

	const struct resolver_address *a = resolver_next(rc->resolver);
	if (a == nullptr) {
		char msg[256];

		if (rc->last_error.empty()) {
			snprintf(msg, sizeof(msg),
				 "Host '%s' has no address",
				 rc->host);
		} else {
			snprintf(msg, sizeof(msg),
				 "Failed to connect to host '%s': %s",
				 rc->host, rc->last_error.c_str());
		}

		rc->handler->error(msg, rc->handler_ctx);
		delete rc;
		return;
	}

	async_connect_start(&rc->connect, a->addr, a->addrlen,
			    *rc);
}

void
async_rconnect_start(AsyncResolveConnect **rcp,
		     const char *host, unsigned port,
		     const AsyncResolveConnectHandler *handler, void *ctx)
{
	struct resolver *r = resolver_new(host, port);
	if (host == nullptr)
		host = "[default]";

	if (r == nullptr) {
		char msg[256];
		snprintf(msg, sizeof(msg), "Failed to resolve host '%s'",
			 host);
		handler->error(msg, ctx);
		return;
	}

	auto *rc = new AsyncResolveConnect(host, r, *handler, ctx);
	*rcp = rc;

	async_rconnect_next(rc);
}

void
async_rconnect_cancel(AsyncResolveConnect *rc)
{
	delete rc;
}
