// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "co/Task.hxx"
#include "co/InvokeTask.hxx"

#include <gtest/gtest.h>

#include <cassert>

/*
static Co::Task<int>
TheLazyTask(int i)
{
	co_return i;
}
*/

static Co::EagerTask<int>
TheEagerTask(int i)
{
	//co_return co_await TheLazyTask(i);
	co_return i;
}

static Co::InvokeTask
RunEagerTask(int i, int &result)
{
	result = co_await TheEagerTask(i);
}

TEST(Task, EagerTask)
{
	int value = -1;
	std::exception_ptr error;

	auto task = RunEagerTask(42, value);
	task.Start({&error, [](void *error_p, std::exception_ptr &&_error) noexcept {
		auto &error_r = *(std::exception_ptr *)error_p;
		error_r = std::move(_error);
	}});

	ASSERT_FALSE(error);
	ASSERT_EQ(value, 42);
}

static const int foo = 42;

static Co::EagerTask<const int &>
ReferenceTask()
{
	co_return foo;
}

static Co::InvokeTask
RunReferenceTask(const int *&result)
{
	result = &co_await ReferenceTask();
}

TEST(Task, Reference)
{
	const int *result = nullptr;
	std::exception_ptr error;

	auto task = RunReferenceTask(result);
	task.Start({&error, [](void *error_p, std::exception_ptr &&_error) noexcept {
		auto &error_r = *(std::exception_ptr *)error_p;
		error_r = std::move(_error);
	}});

	ASSERT_FALSE(error);
	ASSERT_EQ(result, &foo);
}
