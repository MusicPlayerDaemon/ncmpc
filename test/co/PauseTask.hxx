// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "co/AwaitableHelper.hxx"

#include <cassert>

namespace Co {

class PauseTask {
	std::coroutine_handle<> continuation;

	bool resumed = false;

	friend Co::AwaitableHelper<PauseTask, false>;

	struct Awaitable : Co::AwaitableHelper<PauseTask, false> {
		~Awaitable() noexcept {
			task.continuation = {};
               }
	};

public:
	Awaitable operator co_await() noexcept {
		return {*this};
	}

	bool IsAwaited() const noexcept {
		return (bool)continuation;
	}

	void Resume() noexcept {
		assert(!resumed);

		resumed = true;

		if (continuation)
			continuation.resume();
	}

private:
	bool IsReady() const noexcept {
		return resumed;
	}

	void TakeValue() const noexcept {}
};

} // namespace Co
