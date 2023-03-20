// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef BASIC_COLORS_HXX
#define BASIC_COLORS_HXX

/**
 * Parse an ncurses color name.
 *
 * @return the COLOR_* integer value or -1 on error
 */
[[gnu::pure]]
short
ParseBasicColorName(const char *name) noexcept;

/**
 * Like ParseBasicColorName(), but also allow numeric colors.
 *
 * @return the color integer value or -1 on error
 */
[[gnu::pure]]
short
ParseColorNameOrNumber(const char *s) noexcept;

#endif
