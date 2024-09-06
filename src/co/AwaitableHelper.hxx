// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "Compat.hxx"

#include <exception> // for std::rethrow_exception()

namespace Co {

/**
 * This class provides some common boilerplate code for implementing
 * an awaitable for a coroutine task.  The task must have the field
 * "continuation" and the methods IsReady() and TakeValue().  If
 * #rethrow_error is true, then it must also have an "error" field.
 */
template<typename T,
	 bool rethrow_error=true>
class AwaitableHelper {
protected:
	T &task;

public:
	constexpr AwaitableHelper(T &_task) noexcept
		:task(_task) {}

	[[nodiscard]]
	constexpr bool await_ready() const noexcept {
		return task.IsReady();
	}

	void await_suspend(std::coroutine_handle<> _continuation) noexcept {
		task.continuation = _continuation;
	}

	decltype(auto) await_resume() {
		if constexpr (rethrow_error)
			if (this->task.error)
				std::rethrow_exception(this->task.error);

		return this->task.TakeValue();
	}
};

} // namespace Co

