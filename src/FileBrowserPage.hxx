// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_FILE_BROWSER_PAGE_HXX
#define NCMPC_FILE_BROWSER_PAGE_HXX

struct mpdclient;
struct mpd_song;
struct PageMeta;
class ScreenManager;

extern const PageMeta screen_browse;

bool
screen_file_goto_song(ScreenManager &_screen, struct mpdclient &c,
		      const struct mpd_song &song) noexcept;

#endif
