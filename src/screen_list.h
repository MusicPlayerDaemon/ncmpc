/*
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
 *
 */

#ifndef SCREEN_LIST_H
#define SCREEN_LIST_H

#include "ncmpc.h"

#include <ncurses.h>

#define SCREEN_PLAYLIST_ID 0
#define SCREEN_BROWSE_ID 1
#define SCREEN_ARTIST_ID 2
#define SCREEN_HELP_ID 100
#define SCREEN_KEYDEF_ID 101
#define SCREEN_SEARCH_ID 103
#define SCREEN_LYRICS_ID 104

extern const struct screen_functions screen_playlist;
extern const struct screen_functions screen_browse;
#ifdef ENABLE_ARTIST_SCREEN
extern const struct screen_functions screen_artist;
#endif
extern const struct screen_functions screen_help;
#ifdef ENABLE_SEARCH_SCREEN
extern const struct screen_functions screen_search;
#endif
#ifdef ENABLE_KEYDEF_SCREEN
extern const struct screen_functions screen_keydef;
#endif
#ifdef ENABLE_LYRICS_SCREEN
extern const struct screen_functions screen_lyrics;
#endif

void
screen_list_init(WINDOW *w, unsigned cols, unsigned rows);

void
screen_list_exit(void);

void
screen_list_resize(unsigned cols, unsigned rows);

int
screen_get_id_by_index(unsigned i);

const char *
screen_get_name(unsigned i);

int
screen_get_id(const char *name);

const struct screen_functions *
screen_get_functions(unsigned i);

int
lookup_mode(int id);

#endif
