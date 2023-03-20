// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_EDIT_PLAYLIST_PAGE_HXX
#define NCMPC_EDIT_PLAYLIST_PAGE_HXX

struct PageMeta;
class ScreenManager;

extern const PageMeta edit_playlist_page;

void
EditPlaylist(ScreenManager &_screen, struct mpdclient &c,
	     const char *name) noexcept;

#endif
