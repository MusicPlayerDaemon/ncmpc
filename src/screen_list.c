/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#include "screen_list.h"
#include "screen.h"

#include <string.h>

static const struct
{
	const char *name;
	const struct screen_functions *functions;
} screens[] = {
	{ "playlist", &screen_playlist },
	{ "browse", &screen_browse },
#ifdef ENABLE_ARTIST_SCREEN
	{ "artist", &screen_artist },
#endif
#ifdef ENABLE_HELP_SCREEN
	{ "help", &screen_help },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ "search", &screen_search },
#endif
#ifdef ENABLE_SONG_SCREEN
	{ "song", &screen_song },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ "keydef", &screen_keydef },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ "lyrics", &screen_lyrics },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ "outputs", &screen_outputs },
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

const char *
screen_get_name(const struct screen_functions *sf)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i)
		if (screens[i].functions == sf)
			return screens[i].name;

	return NULL;
}

const struct screen_functions *
screen_lookup_name(const char *name)
{
	unsigned i;

	for (i = 0; i < NUM_SCREENS; ++i)
		if (strcmp(name, screens[i].name) == 0)
			return screens[i].functions;

	return NULL;
}
