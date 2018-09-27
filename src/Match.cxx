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

#include <assert.h>
#include <string.h>

MatchExpression::~MatchExpression() noexcept
{
#ifdef HAVE_PCRE
	pcre_free(re);
#endif
}

bool
MatchExpression::Compile(const char *src, bool anchor) noexcept
{
#ifndef HAVE_PCRE
	assert(expression == nullptr);

	expression = src;
	length = strlen(expression);
	anchored = anchor;

	return true;
#else
	assert(re == nullptr);

	int options = PCRE_CASELESS|PCRE_DOTALL|PCRE_NO_AUTO_CAPTURE;
	if (anchor)
		options |= PCRE_ANCHORED;

	const char *error_string;
	int error_offset;
	re = pcre_compile(src, options, &error_string, &error_offset, nullptr);
	return re != nullptr;
#endif
}

bool
MatchExpression::operator()(const char *line) const noexcept
{
#ifdef NCMPC_MINI
	assert(expression != nullptr);

	return anchored
		? strncasecmp(line, expression, length) == 0
		: strstr(line, expression) != nullptr;
#else
	assert(re != nullptr);

	return pcre_exec(re, nullptr, line, strlen(line),
			 0, 0, nullptr, 0) >= 0;
#endif
}
