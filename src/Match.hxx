// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef MATCH_H
#define MATCH_H

#include "config.h"

#ifdef HAVE_PCRE
#include <pcre2.h>
#else
#include <stddef.h>
#endif

class MatchExpression {
#ifndef HAVE_PCRE
	const char *expression;
	size_t length;
	bool anchored;
#else
	pcre2_code_8 *re = nullptr;
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
