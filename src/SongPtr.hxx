// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef SONG_PTR_HXX
#define SONG_PTR_HXX

#include "Deleter.hxx"

#include <memory>

using SongPtr = std::unique_ptr<struct mpd_song, LibmpdclientDeleter>;

#endif
