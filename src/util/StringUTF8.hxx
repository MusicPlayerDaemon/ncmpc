/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#ifndef STRING_UTF8_HXX
#define STRING_UTF8_HXX

#include "Compiler.h"

#include <stddef.h>

gcc_pure
int
CollateUTF8(const char *a, const char *b);

/**
 * Returns the number of terminal cells occupied by this string.
 */
gcc_pure
unsigned
utf8_width(const char *str);

/**
 * Find the first character which doesn't fully fit into the given width.
 *
 * @param s the start of the string
 */
gcc_pure
const char *
AtWidthUTF8(const char *p, size_t length, size_t width) noexcept;

#endif
