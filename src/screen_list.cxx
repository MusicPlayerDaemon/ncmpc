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

#include "screen_list.hxx"
#include "screen_interface.hxx"
#include "screen.hxx"
#include "screen_help.hxx"
#include "screen_queue.hxx"
#include "screen_file.hxx"
#include "screen_artist.hxx"
#include "screen_search.hxx"
#include "screen_song.hxx"
#include "screen_keydef.hxx"
#include "screen_lyrics.hxx"
#include "screen_outputs.hxx"
#include "screen_chat.hxx"

#include <string.h>

static const struct
{
	const char *name;
	const struct screen_functions *functions;
} screens[] = {
	{ "playlist", &screen_queue },
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
#ifdef ENABLE_CHAT_SCREEN
	{ "chat", &screen_chat },
#endif
};

const char *
screen_get_name(const struct screen_functions *sf)
{
	for (unsigned i = 0; i < G_N_ELEMENTS(screens); ++i)
		if (screens[i].functions == sf)
			return screens[i].name;

	return nullptr;
}

const struct screen_functions *
screen_lookup_name(const char *name)
{
	for (unsigned i = 0; i < G_N_ELEMENTS(screens); ++i)
		if (strcmp(name, screens[i].name) == 0)
			return screens[i].functions;

	return nullptr;
}
