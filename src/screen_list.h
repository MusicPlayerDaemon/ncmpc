/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef SCREEN_LIST_H
#define SCREEN_LIST_H

#include "config.h"
#include "ncmpc_curses.h"

struct screen_functions;

void
screen_list_init(WINDOW *w, unsigned cols, unsigned rows);

void
screen_list_exit(void);

void
screen_list_resize(unsigned cols, unsigned rows);

const char *
screen_get_name(const struct screen_functions *sf);

const struct screen_functions *
screen_lookup_name(const char *name);

#endif
