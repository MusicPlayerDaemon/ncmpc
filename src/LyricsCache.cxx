/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "LyricsCache.hxx"
#include "io/Path.hxx"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static std::string
GetLyricsCacheDirectory() noexcept
{
	const char *home = getenv("HOME");
	if (home == nullptr)
		return {};

	return BuildPath(home, ".lyrics");
}

LyricsCache::LyricsCache() noexcept
	:directory(GetLyricsCacheDirectory())
{
}

std::string
LyricsCache::MakePath(const char *artist, const char *title) const noexcept
{
	if (!IsAvailable())
		return {};

	char filename[1024];
	int length = snprintf(filename, sizeof(filename), "%s - %s.txt",
			      artist, title);
	if (length <= 0 || size_t(length) >= sizeof(filename))
		/* too long for the buffer, bail out */
		return {};

	return BuildPath(directory.c_str(), filename);
}

bool
LyricsCache::Exists(const char *artist, const char *title) const noexcept
{
	const auto path = MakePath(artist, title);
	if (path.empty())
		return false;

	struct stat result;
	return (stat(path.c_str(), &result) == 0);
}

FILE *
LyricsCache::Save(const char *artist, const char *title) noexcept
{
	if (!IsAvailable())
		return nullptr;

	mkdir(directory.c_str(), S_IRWXU);

	const auto path = MakePath(artist, title);
	return fopen(path.c_str(), "w");
}

bool
LyricsCache::Delete(const char *artist, const char *title) noexcept
{
	const auto path = MakePath(artist, title);
	if (path.empty())
		return false;

	return unlink(path.c_str()) == 0;
}
