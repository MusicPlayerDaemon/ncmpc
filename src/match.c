/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#include "match.h"
#include "charset.h"

#include <glib.h>
#include <string.h>
#include <ctype.h>

static char *
locale_casefold(const char *src)
{
	char *utf8 = locale_to_utf8(src);
	char *folded = g_utf8_casefold(utf8, -1);

	g_free(utf8);
	return folded;
}

GRegex *
compile_regex(const char *src, bool anchor)
{
	GRegex *regex;
	GRegexCompileFlags compile_flags;
	char *src_folded = locale_casefold(src);

	compile_flags = G_REGEX_CASELESS | G_REGEX_DOTALL | G_REGEX_OPTIMIZE;
	if (anchor)
		compile_flags |= G_REGEX_ANCHORED;

	regex = g_regex_new ((const gchar*)src_folded, compile_flags, 0, NULL);

	g_free(src_folded);

	return regex;
}

bool
match_regex(GRegex *regex, const char *line)
{
	GMatchInfo *match_info;
	bool match;
	char *line_folded = locale_casefold(line);

	g_regex_match(regex, line_folded, 0, &match_info);
	match = (bool)g_match_info_matches(match_info);

	g_match_info_free(match_info);
	g_free(line_folded);

	return match;
}

bool
match_line(const char *line, const char *needle)
{
	char *line_folded = locale_casefold(line);
	char *needle_folded = locale_casefold(needle);

#if GLIB_CHECK_VERSION(2,14,0)
	bool ret = (bool)g_regex_match_simple((const gchar*)needle_folded,
			(const gchar*)line_folded,
			G_REGEX_CASELESS | G_REGEX_DOTALL | G_REGEX_OPTIMIZE,
			0);
#else
	bool ret = strstr(line_folded, needle_folded) != NULL;
#endif

	g_free(line_folded);
	g_free(needle_folded);

	return ret;
}
