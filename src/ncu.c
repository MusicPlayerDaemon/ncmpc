/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "ncu.h"
#include "config.h"
#include "ncmpc_curses.h"

#ifdef ENABLE_COLORS
#include "colors.h"
#endif

#ifdef HAVE_GETMOUSE
#include "options.h"
#endif

static SCREEN *ncu_screen;

void
ncu_init(void)
{
	/* initialize the curses library */
	ncu_screen = newterm(NULL, stdout, stdin);

	/* initialize color support */
#ifdef ENABLE_COLORS
	colors_start();
#endif

	/* tell curses not to do NL->CR/NL on output */
	nonl();

	/* don't echo input */
	noecho();

	/* set cursor invisible */
	curs_set(0);

	/* enable extra keys */
	keypad(stdscr, TRUE);

	/* initialize mouse support */
#ifdef HAVE_GETMOUSE
	if (options.enable_mouse)
		mousemask(ALL_MOUSE_EVENTS, NULL);
#endif

	refresh();
}

void
ncu_deinit(void)
{
	endwin();

	delscreen(ncu_screen);
}
