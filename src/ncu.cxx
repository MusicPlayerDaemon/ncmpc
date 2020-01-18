/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ncu.hxx"
#include "config.h"

#ifdef ENABLE_COLORS
#include "Styles.hxx"
#endif

#ifdef HAVE_GETMOUSE
#include "Options.hxx"
#endif

#include <curses.h>

#ifdef NCURSES_VERSION
#include <string.h>
#endif

static SCREEN *ncu_screen;

void
ncu_init()
{
	/* initialize the curses library */
	ncu_screen = newterm(nullptr, stdout, stdin);

	/* initialize color support */
#ifdef ENABLE_COLORS
	ApplyStyles();
#endif

	/* tell curses not to do NL->CR/NL on output */
	nonl();

	/* don't echo input */
	noecho();

	/* set cursor invisible */
	curs_set(0);

	/* enable extra keys */
	keypad(stdscr, true);

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
#endif

	/* initialize mouse support */
#ifdef HAVE_GETMOUSE
	if (options.enable_mouse)
		mousemask(ALL_MOUSE_EVENTS, nullptr);
#endif

	refresh();
}

void
ncu_deinit()
{
	endwin();

	delscreen(ncu_screen);
}
