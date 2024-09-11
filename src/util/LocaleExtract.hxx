// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cwchar>
#include <string_view>
#include <utility>

/**
 * Find the first character in the string.
 *
 * @return a pointer to the next character and first character's value
 */
[[gnu::pure]]
std::pair<const char *, wchar_t>
ExtractFirstCharMB(std::string_view s, wchar_t fallback) noexcept;

/**
 * Find the last character in the string.
 *
 * @return a pointer to the last character and its value
 */
[[gnu::pure]]
std::pair<const char *, wchar_t>
ExtractLastCharMB(std::string_view s, wchar_t fallback) noexcept;
