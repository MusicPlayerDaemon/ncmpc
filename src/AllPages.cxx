// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "AllPages.hxx"
#include "PageMeta.hxx"
#include "SwitchPage.hxx"
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
#include "ConnectionsPage.hxx"
#include "ChatPage.hxx"
#include "config.h"

#include <iterator>

using std::string_view_literals::operator""sv;

constexpr const PageMeta *all_pages[] = {
	&switch_page,
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
#ifdef ENABLE_CONNECTIONS_PAGE
	&connections_page,
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
	nullptr
};

std::size_t
GetPageCount() noexcept
{
	return std::size(all_pages) - 1;
}

const PageMeta *
screen_lookup_name(std::string_view name) noexcept
{
	for (const PageMeta *const*i = all_pages; *i != nullptr; ++i)
		if (name == (*i)->name)
			return *i;

#ifdef ENABLE_LIBRARY_PAGE
	/* compatibility with 0.32 and older */
	if (name == "artist"sv)
		return &library_page;
#endif

	return nullptr;
}

const PageMeta *
PageByCommand(Command cmd) noexcept
{
	for (const PageMeta *const*i = all_pages; *i != nullptr; ++i)
		if ((*i)->command == cmd)
			return *i;

	return nullptr;
}
