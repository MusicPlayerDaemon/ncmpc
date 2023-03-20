// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_SONG_PAGE_HXX
#define NCMPC_SONG_PAGE_HXX

#include "config.h"

#ifdef ENABLE_SONG_SCREEN

struct mpdclient;
struct mpd_song;
struct PageMeta;
class ScreenManager;

extern const PageMeta screen_song;

void
screen_song_switch(ScreenManager &_screen, struct mpdclient &c,
		   const struct mpd_song &song) noexcept;

#endif /* ENABLE_SONG_SCREEN */

#endif
