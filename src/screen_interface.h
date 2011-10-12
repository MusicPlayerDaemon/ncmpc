/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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
#include "command.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

struct mpdclient;

struct screen_functions {
	void (*init)(WINDOW *w, int cols, int rows);
	void (*exit)(void);
	void (*open)(struct mpdclient *c);
	void (*close)(void);
	void (*resize)(int cols, int rows);
	void (*paint)(void);
	void (*update)(struct mpdclient *c);
	bool (*cmd)(struct mpdclient *c, command_t cmd);
	const char *(*get_title)(char *s, size_t size);
};

#endif
