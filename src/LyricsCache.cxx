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

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void
LyricsCache::MakePath(char *path, size_t size,
		      const char *artist, const char *title) const noexcept
{
	snprintf(path, size, "%s/.lyrics/%s - %s.txt",
		 getenv("HOME"), artist, title);
}

bool
LyricsCache::Exists(const char *artist, const char *title) const noexcept
{
	char path[1024];
	MakePath(path, 1024, artist, title);

	struct stat result;
	return (stat(path, &result) == 0);
}

FILE *
LyricsCache::Save(const char *artist, const char *title) noexcept
{
	char path[1024];
	snprintf(path, 1024, "%s/.lyrics",
		 getenv("HOME"));
	mkdir(path, S_IRWXU);

	MakePath(path, 1024, artist, title);

	return fopen(path, "w");
}

bool
LyricsCache::Delete(const char *artist, const char *title) noexcept
{
	char path[1024];
	MakePath(path, 1024, artist, title);
	return unlink(path) == 0;
}
