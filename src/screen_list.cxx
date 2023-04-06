// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_list.hxx"
#include "PageMeta.hxx"
#include "HelpPage.hxx"
#include "QueuePage.hxx"
#include "FileBrowserPage.hxx"
#include "LibraryPage.hxx"
#include "SearchPage.hxx"
#include "SongPage.hxx"
#include "KeyDefPage.hxx"
#include "EditPlaylistPage.hxx"
#include "LyricsPage.hxx"
#include "OutputsPage.hxx"
#include "ChatPage.hxx"
#include "config.h"
#include "util/StringAPI.hxx"

#include <iterator>

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
#ifdef ENABLE_PLAYLIST_EDITOR
	&edit_playlist_page,
#endif
};

const PageMeta *
GetPageMeta(unsigned i) noexcept
{
	return i < std::size(screens)
		   ? screens[i]
		   : nullptr;
}

const PageMeta *
screen_lookup_name(const char *name) noexcept
{
	for (const auto *i : screens)
		if (StringIsEqual(name, i->name))
			return i;

#ifdef ENABLE_LIBRARY_PAGE
	/* compatibility with 0.32 and older */
	if (StringIsEqual(name, "artist"))
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
