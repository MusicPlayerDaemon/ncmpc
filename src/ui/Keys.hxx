// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <curses.h>

#define KEY_CTL(x) ((x) & 0x1f) /* KEY_CTL(A) == ^A == \1 */

enum : int {
	KEY_BACKSPACE2 = 8,
	KEY_TAB = 9,
	KEY_LINEFEED = '\n',
	KEY_RETURN = '\r',
	KEY_ESCAPE = 0x1b,
	KEY_BACKSPACE3 = 0x7f,
};

constexpr bool
IsFKey(int key) noexcept
{
	return key >= KEY_F(0) && key <= KEY_F(63);
}

constexpr bool
IsBackspace(int key) noexcept
{
	return key == KEY_BACKSPACE ||
	       key == KEY_BACKSPACE2 ||
	       key == KEY_BACKSPACE3;
}
