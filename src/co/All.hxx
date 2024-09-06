// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "UniqueHandle.hxx"

#include <cassert>
#include <exception> // for std::terminate()
#include <tuple>
#include <type_traits>
#include <utility>

namespace Co {

/**
 * A task that becomes ready when all tasks are ready.  It does not
 * pay attention to exceptions thrown by these tasks, and it will not
 * obtain the results.  After this task completes, it is up to the
 * caller to co_wait all individual tasks.
 */
template<typename... Tasks>
class All final {

	/**
	 * A task for Item::OnReady().  It allows set a continuation
	 * for final_suspend(), which is needed to resume the calling
	 * coroutine after all parameter tasks are done.
	 */
	class CompletionTask final {
	public:
		struct promise_type final {
			std::coroutine_handle<> continuation;

			[[nodiscard]]
			auto initial_suspend() noexcept {
				return std::suspend_always{};
			}

			void return_void() noexcept {
			}

			struct final_awaitable {
				[[nodiscard]]
				bool await_ready() const noexcept {
					return false;
				}

				[[nodiscard]]
				std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> coro) noexcept {
					const auto &promise = coro.promise();
					return promise.continuation;
				}

				void await_resume() noexcept {
				}
			};

			[[nodiscard]]
			auto final_suspend() noexcept {
				return final_awaitable{};
			}

			[[nodiscard]]
			auto get_return_object() noexcept {
				return CompletionTask(std::coroutine_handle<promise_type>::from_promise(*this));
			}

			void unhandled_exception() noexcept {
				std::terminate();
			}
		};

	private:
		UniqueHandle<promise_type> coroutine;

		[[nodiscard]]
		explicit CompletionTask(std::coroutine_handle<promise_type> _coroutine) noexcept
			:coroutine(_coroutine) {}

	public:
		[[nodiscard]]
		CompletionTask() = default;

		operator std::coroutine_handle<>() const noexcept {
			return coroutine.get();
		}

		/**
		 * Set the coroutine that shall be resumed when this
		 * task finishes.
		 */
		void SetContinuation(std::coroutine_handle<> c) noexcept {
			assert(c);
			assert(!c.done());

			auto &promise = coroutine->promise();
			promise.continuation = c;
		}
	};

	/**
	 * An individual task that was passed to this class; it
	 * contains the awaitable obtained by "operator co_await".
	 */
	template<typename Task>
	struct Item {
		All *parent;

		using Awaitable = decltype(std::declval<Task>().operator co_await());
		Awaitable awaitable;

		CompletionTask task;

		bool ready;

		explicit Item(Awaitable _awaitable) noexcept
			:awaitable(_awaitable),
			 ready(awaitable.await_ready()) {}

		void SetParent(All &_parent) noexcept {
			parent = &_parent;
		}

		[[nodiscard]]
		bool await_ready() const noexcept {
			return ready;
		}

		void await_suspend() noexcept {
			if (ready)
				return;

			/* construct a callback task that will be
			   invoked as soon as the given task
			   completes */
			task = OnReady();
			const auto c = awaitable.await_suspend(task);
			c.resume();
		}

	private:
		/**
		 * Completion callback for the given task.  It calls
		 * All::OnReady() to obtain a continuation that will
		 * be resumed by CompletionTask::final_suspend().
		 */
		[[nodiscard]]
		CompletionTask OnReady() noexcept {
			assert(!ready);

			ready = true;

			task.SetContinuation(parent->OnReady());
			co_return;
		}
	};

	std::tuple<Item<Tasks>...> awaitables;

	std::coroutine_handle<> continuation;

public:
	template<typename... Args>
	[[nodiscard]]
	All(Tasks&... tasks) noexcept
		:awaitables(tasks.operator co_await()...) {

		/* this kludge is necessary because we can't pass
		   another parameter to std::tuple */
		std::apply([&](auto &...i){
			(i.SetParent(*this), ...);
		}, awaitables);
	}

	[[nodiscard]]
	bool await_ready() const noexcept {
		/* this task is ready when all given tasks are
		   ready */
		return std::apply([&](const auto &...i){
			return (i.await_ready() && ...);
		}, awaitables);
	}

	void await_suspend(std::coroutine_handle<> _continuation) noexcept {
		/* at least one task is not yet ready - call
		   await_suspend() on not-yet-ready tasks to install
		   the completion callback */

		continuation = _continuation;

		std::apply([&](auto &...i){
			(i.await_suspend(), ...);
		}, awaitables);
	}

	void await_resume() noexcept {
	}

private:
	[[nodiscard]]
	std::coroutine_handle<> OnReady() noexcept {
		assert(continuation);

		/* if all tasks are ready, we can resume our
		   continuation, otherwise do nothing */
		return await_ready()
			? continuation
			: std::noop_coroutine();
	}
};

} // namespace Co
