// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef SAVE_PLAYLIST_H
#define SAVE_PLAYLIST_H

struct mpdclient;

int
playlist_save(struct mpdclient *c, const char *name,
	      const char *defaultname) noexcept;

#endif
