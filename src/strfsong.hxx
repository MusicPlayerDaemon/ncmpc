// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <span>
#include <string_view>

struct mpd_song;
class TagMask;

[[nodiscard]] [[gnu::pure]]
std::string_view
strfsong(std::span<char> buffer, const char *format,
	 const struct mpd_song &song) noexcept;

/**
 * Check which tags are referenced by the given song format.
 */
[[gnu::pure]]
TagMask
SongFormatToTagMask(const char *format) noexcept;
