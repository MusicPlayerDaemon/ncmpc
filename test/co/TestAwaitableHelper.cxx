// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "co/AwaitableHelper.hxx"
#include "co/Task.hxx"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

/**
 * The "error" field which #MockTask has only if rethrow_error=true.
 */
struct MockTaskError {
	std::exception_ptr error;
};

struct MockTaskNoError {};

/**
 * A minimal task implementing the interface expected by
 * Co::AwaitableHelper.
 */
template<bool rethrow_error, bool throwing_take_value=false>
class MockTask
	: public std::conditional_t<rethrow_error, MockTaskError, MockTaskNoError>
{
	using Awaitable = Co::AwaitableHelper<MockTask, rethrow_error>;
	friend Awaitable;

	std::coroutine_handle<> continuation;

	int value = 0;

	bool ready = false;

public:
	[[nodiscard]]
	Awaitable operator co_await() noexcept {
		return *this;
	}

	void SetReady(int _value) noexcept {
		assert(!ready);

		value = _value;
		ready = true;

		if (continuation)
			std::exchange(continuation, {}).resume();
	}

	void SetError(std::exception_ptr _error) noexcept requires rethrow_error {
		assert(!ready);

		this->error = std::move(_error);
		ready = true;

		if (continuation)
			std::exchange(continuation, {}).resume();
	}

private:
	bool IsReady() const noexcept {
		return ready;
	}

	int TakeValue() noexcept(!throwing_take_value) {
		if constexpr (throwing_take_value)
			throw std::runtime_error{"TakeValue() failed"};

		return value;
	}
};

/* await_resume() may throw if errors are rethrown ... */
static_assert(!noexcept(std::declval<Co::AwaitableHelper<MockTask<true>, true>>().await_resume()));

/* ... or if TakeValue() can throw ... */
static_assert(!noexcept(std::declval<Co::AwaitableHelper<MockTask<false, true>, false>>().await_resume()));

/* ... but not if neither can throw */
static_assert(noexcept(std::declval<Co::AwaitableHelper<MockTask<false>, false>>().await_resume()));

static Co::EagerTask<void>
Waiter(auto &task, std::optional<int> &value_r, std::exception_ptr &error_r) noexcept
{
	assert(!value_r);
	assert(!error_r);

	try {
		value_r = co_await task;
	} catch (...) {
		error_r = std::current_exception();
	}
}

} // anonymous namespace

TEST(AwaitableHelper, ReadyEarly)
{
	MockTask<true> task;
	task.SetReady(42);

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(error);
	ASSERT_TRUE(value);
	EXPECT_EQ(*value, 42);
}

TEST(AwaitableHelper, ReadyLate)
{
	MockTask<true> task;

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(value);
	EXPECT_FALSE(error);

	task.SetReady(42);

	EXPECT_FALSE(error);
	ASSERT_TRUE(value);
	EXPECT_EQ(*value, 42);
}

/**
 * With rethrow_error=true, a pending error is rethrown by
 * await_resume().
 */
TEST(AwaitableHelper, RethrowError)
{
	MockTask<true> task;

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(value);
	EXPECT_FALSE(error);

	task.SetError(std::make_exception_ptr(std::runtime_error{"Error"}));

	EXPECT_FALSE(value);
	ASSERT_TRUE(error);
	EXPECT_THROW(std::rethrow_exception(error), std::runtime_error);
}

/**
 * With rethrow_error=false, there is no error check at all.
 */
TEST(AwaitableHelper, NoRethrowError)
{
	MockTask<false> task;

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(value);
	EXPECT_FALSE(error);

	task.SetReady(42);

	EXPECT_FALSE(error);
	ASSERT_TRUE(value);
	EXPECT_EQ(*value, 42);
}

/**
 * Regression test: an exception thrown by TakeValue() must be
 * propagated to the caller instead of calling std::terminate().
 */
TEST(AwaitableHelper, ThrowingTakeValue)
{
	MockTask<true, true> task;

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(value);
	EXPECT_FALSE(error);

	task.SetReady(42);

	EXPECT_FALSE(value);
	ASSERT_TRUE(error);
	EXPECT_THROW(std::rethrow_exception(error), std::runtime_error);
}

/**
 * Like ThrowingTakeValue, but with rethrow_error=false; this is the
 * case that used to be declared `noexcept`, causing std::terminate()
 * to be called.
 */
TEST(AwaitableHelper, ThrowingTakeValueNoRethrowError)
{
	MockTask<false, true> task;

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(value);
	EXPECT_FALSE(error);

	task.SetReady(42);

	EXPECT_FALSE(value);
	ASSERT_TRUE(error);
	EXPECT_THROW(std::rethrow_exception(error), std::runtime_error);
}

/**
 * Like ThrowingTakeValueNoRethrowError, but the task is already ready
 * when it is awaited, i.e. the coroutine is never suspended.
 */
TEST(AwaitableHelper, ThrowingTakeValueReadyEarly)
{
	MockTask<false, true> task;
	task.SetReady(42);

	std::optional<int> value;
	std::exception_ptr error;
	const auto w = Waiter(task, value, error);

	EXPECT_FALSE(value);
	ASSERT_TRUE(error);
	EXPECT_THROW(std::rethrow_exception(error), std::runtime_error);
}
