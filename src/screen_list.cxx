/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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
#include "PageMeta.hxx"
#include "HelpPage.hxx"
#include "QueuePage.hxx"
#include "FileBrowserPage.hxx"
#include "LibraryPage.hxx"
#include "SearchPage.hxx"
#include "SongPage.hxx"
#include "KeyDefPage.hxx"
#include "LyricsPage.hxx"
#include "OutputsPage.hxx"
#include "ChatPage.hxx"
#include "util/Macros.hxx"
#include "config.h"

#include <string.h>

static const PageMeta *const screens[] = {
#ifdef ENABLE_HELP_SCREEN
	&screen_help,
#endif
	&screen_queue,
	&screen_browse,
#ifdef ENABLE_LIBRARY_PAGE
	&library_page,
#endif
#ifdef ENABLE_SEARCH_SCREEN
	&screen_search,
#endif
#ifdef ENABLE_LYRICS_SCREEN
	&screen_lyrics,
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	&screen_outputs,
#endif
#ifdef ENABLE_CHAT_SCREEN
	&screen_chat,
#endif
#ifdef ENABLE_SONG_SCREEN
	&screen_song,
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	&screen_keydef,
#endif
};

const PageMeta *
GetPageMeta(unsigned i) noexcept
{
	return i < ARRAY_SIZE(screens)
		   ? screens[i]
		   : nullptr;
}

const PageMeta *
screen_lookup_name(const char *name) noexcept
{
	for (const auto *i : screens)
		if (strcmp(name, i->name) == 0)
			return i;

#ifdef ENABLE_LIBRARY_PAGE
	/* compatibility with 0.32 and older */
	if (strcmp(name, "artist") == 0)
		return &library_page;
#endif

	return nullptr;
}

const PageMeta *
PageByCommand(Command cmd) noexcept
{
	for (const auto *i : screens)
		if (i->command == cmd)
			return i;

	return nullptr;
}
