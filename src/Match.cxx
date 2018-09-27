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

#include "Match.hxx"
#include "charset.hxx"

#include <glib.h>

#include <assert.h>

MatchExpression::~MatchExpression() noexcept
{
#ifndef NCMPC_MINI
	if (regex != nullptr)
		g_regex_unref(regex);
#endif
}

bool
MatchExpression::Compile(const char *src, bool anchor) noexcept
{
#ifdef NCMPC_MINI
	assert(expression == nullptr);

	expression = src;
	length = strlen(expression);
	anchored = anchor;

	return true;
#else
	assert(regex == nullptr);

	unsigned compile_flags =
		G_REGEX_CASELESS | G_REGEX_DOTALL | G_REGEX_OPTIMIZE;
	if (anchor)
		compile_flags |= G_REGEX_ANCHORED;

	regex = g_regex_new(LocaleToUtf8(src).c_str(),
			    GRegexCompileFlags(compile_flags),
			    GRegexMatchFlags(0), nullptr);
	return regex != nullptr;
#endif
}

bool
MatchExpression::operator()(const char *line) const noexcept
{
#ifdef NCMPC_MINI
	assert(expression != nullptr);

	return anchored
		? g_ascii_strncasecmp(line, expression, length) == 0
		: strstr(line, expression) != nullptr;
#else
	assert(regex != nullptr);

	GMatchInfo *match_info;
	g_regex_match(regex, LocaleToUtf8(line).c_str(),
		      GRegexMatchFlags(0), &match_info);
	bool match = (bool)g_match_info_matches(match_info);

	g_match_info_free(match_info);
	return match;
#endif
}
