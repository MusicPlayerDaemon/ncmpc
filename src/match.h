/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MATCH_H
#define MATCH_H

#include "config.h"

#include <stdbool.h>

#ifdef NCMPC_MINI

#include <string.h>

static inline bool
match_line(const char *line, const char *needle)
{
	return strstr(line, needle) != NULL;
}
#else

#include <glib.h>

GRegex *
compile_regex(const char *src, bool anchor);

bool
match_regex(GRegex *regex, const char *line);

/**
 * Checks whether the specified line matches the search string.  Case
 * is ignored.
 */
bool
match_line(const char *line, const char *needle);

#endif

#endif
