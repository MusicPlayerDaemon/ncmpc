// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Keys.hxx"

#ifdef NCURSES_VERSION
#include <string.h>
#endif

#ifdef NCURSES_VERSION
int key_control_up, key_control_down, key_control_left, key_control_right;

#ifdef __APPLE__
/* wtf, Apple ships a broken ncurses on macOS (NCURSES_CONST is empty) */
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

/**
 * Wrapper for key_defined(tigetstr()).
 */
[[gnu::pure]]
static int
GetKeyCode(const char *cap_code) noexcept
{
	return key_defined(tigetstr(cap_code));
}

#endif // NCURSES_VERSION

void
InitKeys() noexcept
{
#ifdef NCURSES_VERSION
	/* define Alt-* keys which for some reasons aren't defined by
	   default (tested with ncurses 6.1 on Linux) */

	if (!key_defined("M-^@")) {
		char buffer[8];
		buffer[0] = 033;

		for (int i = 0x80; i <= 0xff; ++i) {
			const char *name = keyname(i);
			if (name != nullptr && name[0] == 'M' &&
			    name[1] == '-' && name[2] != 0 &&
			    (name[3] == 0 || name[4] == 0)) {
				strcpy(buffer + 1, name + 2);
				define_key(buffer, i);
			}
		}
	}

	key_control_up = GetKeyCode("kUP5");
	key_control_down = GetKeyCode("kDN5");
	key_control_left = GetKeyCode("kLFT5");
	key_control_right = GetKeyCode("kRIT5");

#endif // NCURSES_VERSION
}
