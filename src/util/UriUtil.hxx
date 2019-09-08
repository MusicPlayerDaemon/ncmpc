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

#ifndef URI_UTIL_HXX
#define URI_UTIL_HXX

#include "Compiler.h"

#include <string>

/**
 * Determins the last segment of the given URI path, i.e. the portion
 * after the last slash.  May return an empty string if the URI ends
 * with a slash.
 */
gcc_pure
const char *
GetUriFilename(const char *uri);

/**
 * Return the "parent directory" of the given URI path, i.e. the
 * portion up to the last (forward) slash.  Returns an empty string if
 * there is no parent.
 */
gcc_pure
std::string
GetParentUri(const char *uri);

#endif
