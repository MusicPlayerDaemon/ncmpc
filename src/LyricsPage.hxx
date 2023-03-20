// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_LYRICS_PAGE_HXX
#define NCMPC_LYRICS_PAGE_HXX

#include "config.h"

#ifdef ENABLE_LYRICS_SCREEN

struct mpdclient;
struct mpd_song;
struct PageMeta;
class ScreenManager;

extern const PageMeta screen_lyrics;

void
screen_lyrics_switch(ScreenManager &_screen, struct mpdclient &c,
		     const struct mpd_song &song,
		     bool follow);

#endif /* ENABLE_LYRICS_SCREEN */

#endif
