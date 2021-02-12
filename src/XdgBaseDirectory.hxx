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

#ifndef XDG_BASE_DIRECTORY_HXX
#define XDG_BASE_DIRECTORY_HXX

#include "util/Compiler.h"

#include <string>

gcc_const
const char *
GetHomeDirectory() noexcept;

gcc_const
std::string
GetHomeConfigDirectory() noexcept;

gcc_pure
std::string
GetHomeConfigDirectory(const char *package) noexcept;

/**
 * Find or create the directory for writing configuration files.
 *
 * @return the absolute path; an empty string indicates that no
 * directory could be created
 */
std::string
MakeUserConfigPath(const char *filename) noexcept;

gcc_const
std::string
GetHomeCacheDirectory() noexcept;

gcc_pure
std::string
GetHomeCacheDirectory(const char *package) noexcept;

#endif
