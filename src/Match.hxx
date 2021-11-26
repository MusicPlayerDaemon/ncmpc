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

#ifndef MATCH_H
#define MATCH_H

#include "config.h"

#ifdef HAVE_PCRE
#include <pcre.h>
#else
#include <stddef.h>
#endif

class MatchExpression {
#ifndef HAVE_PCRE
	const char *expression;
	size_t length;
	bool anchored;
#else
	pcre *re = nullptr;
#endif

public:
	MatchExpression() = default;
	~MatchExpression() noexcept;

	MatchExpression(const MatchExpression &) = delete;
	MatchExpression &operator=(const MatchExpression &) = delete;

	bool Compile(const char *src, bool anchor) noexcept;

	[[gnu::pure]]
	bool operator()(const char *line) const noexcept;
};

#endif
