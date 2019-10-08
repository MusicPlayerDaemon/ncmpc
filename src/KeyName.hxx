/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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

#ifndef KEY_NAME_HXX
#define KEY_NAME_HXX

#include "util/Compiler.h"

#include <utility>

/**
 * Parse a string (from the configuration file) to a keycode.
 *
 * @return the keycode and the first unparsed character; -1 indicates
 * error
 */
gcc_pure
std::pair<int, const char *>
ParseKeyName(const char *s) noexcept;

/**
 * Convert a keycode to a canonical string, to be used in the
 * configuration file.
 *
 * The returned pointer is invalidated by the next call.
 */
gcc_pure
const char *
GetKeyName(int key) noexcept;

/**
 * Convert a keycode to a human-readable localized string.
 *
 * The returned pointer is invalidated by the next call.
 */
gcc_pure
const char *
GetLocalizedKeyName(int key) noexcept;

#endif
