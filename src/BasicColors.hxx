// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string_view>

/**
 * Parse an ncurses color name.
 *
 * @return the COLOR_* integer value or -1 on error
 */
[[gnu::pure]]
short
ParseBasicColorName(std::string_view name) noexcept;

/**
 * Like ParseBasicColorName(), but also allow numeric colors.
 *
 * @return the color integer value or -1 on error
 */
[[gnu::pure]]
short
ParseColorNameOrNumber(std::string_view s) noexcept;
