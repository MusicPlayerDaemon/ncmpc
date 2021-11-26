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

#include "Match.hxx"
#include "util/ScopeExit.hxx"

#include <assert.h>
#include <string.h>

MatchExpression::~MatchExpression() noexcept
{
#ifdef HAVE_PCRE
	pcre2_code_free_8(re);
#endif
}

bool
MatchExpression::Compile(const char *src, bool anchor) noexcept
{
#ifndef HAVE_PCRE
	expression = src;
	length = strlen(expression);
	anchored = anchor;

	return true;
#else
	assert(re == nullptr);

	int options = PCRE2_CASELESS|PCRE2_DOTALL|PCRE2_NO_AUTO_CAPTURE;
	if (anchor)
		options |= PCRE2_ANCHORED;

	int error_number;
	PCRE2_SIZE error_offset;
	re = pcre2_compile_8(PCRE2_SPTR8(src), PCRE2_ZERO_TERMINATED,
			     options, &error_number, &error_offset, nullptr);
	return re != nullptr;
#endif
}

bool
MatchExpression::operator()(const char *line) const noexcept
{
#ifndef HAVE_PCRE
	assert(expression != nullptr);

	return anchored
		? strncasecmp(line, expression, length) == 0
		: strstr(line, expression) != nullptr;
#else
	assert(re != nullptr);

	const auto match_data =
		pcre2_match_data_create_from_pattern_8(re, nullptr);
	AtScopeExit(match_data) {
		pcre2_match_data_free_8(match_data);
	};

	return pcre2_match_8(re, (PCRE2_SPTR8)line, strlen(line),
			     0, 0, match_data, nullptr) >= 0;
#endif
}
