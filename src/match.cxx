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

#include "match.hxx"
#include "charset.hxx"
#include "util/ScopeExit.hxx"

#include <glib.h>

static char *
locale_casefold(const char *src)
{
	return g_utf8_casefold(LocaleToUtf8(src).c_str(), -1);
}

GRegex *
compile_regex(const char *src, bool anchor)
{
	unsigned compile_flags =
		G_REGEX_CASELESS | G_REGEX_DOTALL | G_REGEX_OPTIMIZE;
	if (anchor)
		compile_flags |= G_REGEX_ANCHORED;

	char *src_folded = locale_casefold(src);
	AtScopeExit(src_folded) { g_free(src_folded); };

	return g_regex_new((const gchar*)src_folded,
			   GRegexCompileFlags(compile_flags),
			   GRegexMatchFlags(0), nullptr);
}

bool
match_regex(GRegex *regex, const char *line)
{
	char *line_folded = locale_casefold(line);
	AtScopeExit(line_folded) { g_free(line_folded); };

	GMatchInfo *match_info;
	g_regex_match(regex, line_folded, GRegexMatchFlags(0), &match_info);
	bool match = (bool)g_match_info_matches(match_info);

	g_match_info_free(match_info);
	return match;
}

bool
match_line(const char *line, const char *needle)
{
	char *line_folded = locale_casefold(line);
	char *needle_folded = locale_casefold(needle);

	bool ret = (bool)g_regex_match_simple((const gchar*)needle_folded,
					      (const gchar*)line_folded,
					      GRegexCompileFlags(G_REGEX_CASELESS | G_REGEX_DOTALL | G_REGEX_OPTIMIZE),
					      GRegexMatchFlags(0));

	g_free(line_folded);
	g_free(needle_folded);

	return ret;
}
