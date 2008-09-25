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

#include "screen_list.h"
#include "screen.h"

#include <string.h>

static const struct
{
	int id;
	const char *name;
	const struct screen_functions *functions;
} screens[] = {
	{ SCREEN_PLAYLIST_ID, "playlist", &screen_playlist },
	{ SCREEN_BROWSE_ID, "browse", &screen_browse },
#ifdef ENABLE_ARTIST_SCREEN
	{ SCREEN_ARTIST_ID, "artist", &screen_artist },
#endif
	{ SCREEN_HELP_ID, "help", &screen_help },
#ifdef ENABLE_SEARCH_SCREEN
	{ SCREEN_SEARCH_ID, "search", &screen_search },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ SCREEN_KEYDEF_ID, "keydef", &screen_keydef },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ SCREEN_LYRICS_ID, "lyrics", &screen_lyrics },
#endif
};

static const unsigned NUM_SCREENS = sizeof(screens) / sizeof(screens[0]);

void
screen_list_init(WINDOW *w, unsigned cols, unsigned rows)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i) {
		const struct screen_functions *sf = screens[i].functions;

		if (sf->init)
			sf->init(w, cols, rows);
	}
}

void
screen_list_exit(void)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i) {
		const struct screen_functions *sf = screens[i].functions;

		if (sf->exit)
			sf->exit();
	}
}

void
screen_list_resize(unsigned cols, unsigned rows)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i) {
		const struct screen_functions *sf = screens[i].functions;

		if (sf->resize)
			sf->resize(cols, rows);
	}
}

int
screen_get_id_by_index(unsigned i)
{
	assert(i < NUM_SCREENS);

	return screens[i].id;
}

const char *
screen_get_name(unsigned i)
{
	assert(i < NUM_SCREENS);

	return screens[i].name;
}

int
screen_get_id(const char *name)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i)
		if (strcmp(name, screens[i].name) == 0)
			return screens[i].id;

	return -1;
}

const struct screen_functions *
screen_get_functions(unsigned i)
{
	assert(i < NUM_SCREENS);

	return screens[i].functions;
}

int
lookup_mode(int id)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i)
		if (screens[i].id == id)
			return i;

	return -1;
}
