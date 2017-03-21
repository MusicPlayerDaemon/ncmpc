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

#ifndef COLORS_H
#define COLORS_H

#include "config.h"
#include "ncmpc_curses.h"
#include "Compiler.h"

#include <stdbool.h>

enum color {
	COLOR_TITLE = 1,
	COLOR_TITLE_BOLD,
	COLOR_LINE,
	COLOR_LINE_BOLD,
	COLOR_LINE_FLAGS,
	COLOR_LIST,
	COLOR_LIST_BOLD,
	COLOR_PROGRESSBAR,
	COLOR_STATUS,
	COLOR_STATUS_BOLD,
	COLOR_STATUS_TIME,
	COLOR_STATUS_ALERT,
	COLOR_DIRECTORY,
	COLOR_PLAYLIST,
	COLOR_BACKGROUND,
	COLOR_END
};

gcc_pure
int colors_str2color(const char *str);

#ifdef ENABLE_COLORS
bool
colors_assign(const char *name, const char *value);

bool
colors_define(const char *name, short r, short g, short b);

void
colors_start(void);
#endif

void
colors_use(WINDOW *w, enum color id);

#endif /* COLORS_H */
