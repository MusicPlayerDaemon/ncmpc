/* ncmpc
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ncu.h"
#include "colors.h"

void
ncu_init(void)
{
	/* initialize the curses library */
	initscr();

	/* initialize color support */
	colors_start();

	/* tell curses not to do NL->CR/NL on output */
	nonl();

	/*  use raw mode (ignore interrupt,quit,suspend, and flow control ) */
#ifdef ENABLE_RAW_MODE
	//  raw();
#endif

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
}

void
ncu_deinit(void)
{
	endwin();
}
