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

/**
 * Strip dangerous/illegal characters from a file name, to avoid path
 * injection bugs.
 */
static void
SanitizeFilename(char *p) noexcept
{
	for (; *p; ++p) {
		char ch = *p;

		if (ch == '/')
			*p = '-';
	}
}

static std::string
MakePath(const std::string &directory,
	 const char *artist, const char *title) noexcept
{
	if (directory.empty())
		return {};

	char filename[1024];
	int length = snprintf(filename, sizeof(filename), "%s - %s.txt",
			      artist, title);
	if (length <= 0 || size_t(length) >= sizeof(filename))
		/* too long for the buffer, bail out */
		return {};

	SanitizeFilename(filename);

	return BuildPath(directory.c_str(), filename);
}

std::string
LyricsCache::MakePath(const char *artist, const char *title) const noexcept
{
	return ::MakePath(directory, artist, title);
}

gcc_pure
static bool
ExistsFile(const std::string &path) noexcept
{
	if (path.empty())
		return false;

	struct stat result;
	return stat(path.c_str(), &result) == 0;
}

bool
LyricsCache::Exists(const char *artist, const char *title) const noexcept
{
	return ExistsFile(MakePath(artist, title));
}

gcc_pure
static std::string
LoadFile(const std::string &path) noexcept
{
	if (path.empty())
		return {};

	FILE *file = fopen(path.c_str(), "r");
	if (file == nullptr)
		return {};

	constexpr std::size_t MAX_SIZE = 256 * 1024;
	std::string value;

	do {
		char buffer[1024];
		auto nbytes = fread(buffer, 1, sizeof(buffer), file);
		if (nbytes <= 0)
			break;

		value.append(buffer, nbytes);
	} while (value.length() < MAX_SIZE);

	fclose(file);
	return value;
}

std::string
LyricsCache::Load(const char *artist, const char *title) const noexcept
{
	return LoadFile(MakePath(artist, title));
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

static bool
DeleteFile(const std::string &path) noexcept
{
	return !path.empty() && unlink(path.c_str()) == 0;
}

bool
LyricsCache::Delete(const char *artist, const char *title) noexcept
{
	return DeleteFile(MakePath(artist, title));
}
