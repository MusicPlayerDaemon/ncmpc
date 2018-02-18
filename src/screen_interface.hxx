/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#ifndef NCMPC_SCREEN_INTERFACE_H
#define NCMPC_SCREEN_INTERFACE_H

#include "config.h"
#include "command.hxx"
#include "ncmpc_curses.h"

#include <stddef.h>

struct mpdclient;

struct screen_functions {
	void (*init)(WINDOW *w, unsigned cols, unsigned rows);
	void (*exit)();
	void (*open)(struct mpdclient *c);
	void (*close)();
	void (*resize)(unsigned cols, unsigned rows);
	void (*paint)();
	void (*update)(struct mpdclient *c);

	/**
	 * Handle a command.
	 *
	 * @returns true if the command should not be handled by the ncmpc core.
	 */
	bool (*cmd)(struct mpdclient *c, command_t cmd);

#ifdef HAVE_GETMOUSE
	/**
	 * Handle a mouse event.
	 *
	 * @return true if the event was handled (and should not be
	 * handled by the ncmpc core)
	 */
	bool (*mouse)(struct mpdclient *c, int x, int y, mmask_t bstate);
#endif

	const char *(*get_title)(char *s, size_t size);
};

#endif
