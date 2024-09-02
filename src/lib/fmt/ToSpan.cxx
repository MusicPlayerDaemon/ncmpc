// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "ToSpan.hxx"

std::string_view
VFmtTruncate(std::span<char> buffer,
	     fmt::string_view format_str, fmt::format_args args) noexcept
{
	auto [p, _] = fmt::vformat_to_n(buffer.begin(), buffer.size(),
					format_str, args);
	return {buffer.begin(), p};
}
