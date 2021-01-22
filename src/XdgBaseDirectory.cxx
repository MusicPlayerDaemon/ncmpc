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

#include "XdgBaseDirectory.hxx"
#include "config.h"
#include "io/Path.hxx"

#include <stdlib.h>
#include <sys/stat.h>

gcc_pure
static bool
IsDirectory(const char *path) noexcept
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

const char *
GetHomeDirectory() noexcept
{
	return getenv("HOME");
}

std::string
GetHomeConfigDirectory() noexcept
{
	const char *config_home = getenv("XDG_CONFIG_HOME");
	if (config_home != nullptr && *config_home != 0)
		return config_home;

	const char *home = GetHomeDirectory();
	if (home != nullptr)
		return BuildPath(home, ".config");

	return {};
}

std::string
GetHomeConfigDirectory(const char *package) noexcept
{
	const auto dir = GetHomeConfigDirectory();
	if (dir.empty())
		return {};

	return BuildPath(dir, package);
}

std::string
MakeUserConfigPath(const char *filename) noexcept
{
	const auto directory = GetHomeConfigDirectory(PACKAGE);
	if (directory.empty())
		return {};

	return IsDirectory(directory.c_str()) ||
		mkdir(directory.c_str(), 0755) == 0
		? BuildPath(directory, filename)
		: std::string();
}
