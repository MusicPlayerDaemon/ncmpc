// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <span>

#include <stdio.h>

template<typename... Args>
static inline std::string_view
SPrintf(std::span<char> buffer, const char *fmt, Args&&... args) noexcept
{
	int length = snprintf(buffer.data(), buffer.size(), fmt, args...);
	if (length < 0)
		return {};

	return {buffer.data(), static_cast<std::size_t>(length)};
}
