/*
 * (c) 2008 Max Kellermann <max@duempel.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "match.h"
#include "charset.h"

#include <glib.h>
#include <string.h>

static char *
locale_casefold(const char *src)
{
	char *utf8 = locale_to_utf8(src);
	char *folded = g_utf8_casefold(utf8, -1);

	g_free(utf8);
	return folded;
}

bool
match_line(const char *line, const char *needle)
{
	char *line_folded = locale_casefold(line);
	char *needle_folded = locale_casefold(needle);

	bool ret = strstr(line_folded, needle_folded) != NULL;

	g_free(line_folded);
	g_free(needle_folded);

	return ret;
}
