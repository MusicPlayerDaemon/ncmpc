// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef STRFSONG_H
#define STRFSONG_H

#include <stddef.h>

struct mpd_song;
class TagMask;

size_t
strfsong(char *s, size_t max, const char *format,
	 const struct mpd_song *song) noexcept;

/**
 * Check which tags are referenced by the given song format.
 */
[[gnu::pure]]
TagMask
SongFormatToTagMask(const char *format) noexcept;

#endif
