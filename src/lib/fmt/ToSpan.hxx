// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <fmt/core.h>

#include <span>
#include <string_view>

[[nodiscard]] [[gnu::pure]]
std::string_view
VFmtTruncate(std::span<char> buffer,
	     fmt::string_view format_str, fmt::format_args args) noexcept;

template<typename S, typename... Args>
[[nodiscard]] [[gnu::pure]]
inline std::string_view
FmtTruncate(std::span<char> buffer, const S &format_str, Args&&... args) noexcept
{
	return VFmtTruncate(buffer, format_str, fmt::make_format_args(args...));
}
