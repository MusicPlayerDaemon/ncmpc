// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <span>
#include <string>
#include <string_view>

/**
 * Convert #src from UTF-8 to the locale charset, copying into #dest.
 * If #dest is too small, the output will be truncated.
 *
 * @param dest a buffer to write the locale charset string into
 * @param src the UTF-8 source string
 * @return the destination end pointer
 */
char *
ConvertUtf8ToLocaleTruncate(std::span<char> dest, const std::string_view src) noexcept;

/**
 * Convert #src from UTF-8 to the locale charset and return the result
 * as std::string.
 */
[[gnu::pure]]
std::string
ConvertUtf8ToLocaleString(const std::string_view src) noexcept;

/**
 * Convert #src from the locale charset to UTF-8 and return the result
 * as std::string.
 *
 * @return the UTF-8 string
 */
[[gnu::pure]]
std::string
ConvertLocaleToUtf8String(std::string_view src) noexcept;
