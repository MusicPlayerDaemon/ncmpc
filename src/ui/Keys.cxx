// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Keys.hxx"

#ifdef NCURSES_VERSION
#include <string.h>
#endif

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
#endif // NCURSES_VERSION
}
