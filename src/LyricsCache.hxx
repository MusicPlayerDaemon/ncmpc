// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef LYRICS_CACHE_HXX
#define LYRICS_CACHE_HXX

#include <string>

#include <stddef.h>
#include <stdio.h>

class LyricsCache {
	const std::string directory;

	const std::string legacy_directory;

public:
	LyricsCache() noexcept;

	bool IsAvailable() const noexcept {
		return !directory.empty();
	}

	std::string MakePath(const char *artist,
			     const char *title) const noexcept;

	[[gnu::pure]]
	bool Exists(const char *artist, const char *title) const noexcept;

	std::string Load(const char *artist, const char *title) const noexcept;

	FILE *Save(const char *artist, const char *title) noexcept;

	/**
	 * @return true on success
	 */
	bool Delete(const char *artist, const char *title) noexcept;
};

#endif
