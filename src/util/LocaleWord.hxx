// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <string_view>

/**
 * Determine the end of the first word.
 *
 * @param start the start of the string
 */
[[gnu::pure]]
const char *
NextWordMB(std::string_view s) noexcept;

/**
 * Determine the start of the last word.
 *
 * @param start the start of the string
 */
[[gnu::pure]]
const char *
LastWordMB(std::string_view s) noexcept;
