// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

#ifdef HAVE_PCRE
#include <pcre2.h>
#else
#include <stddef.h>
#endif

#include <string_view>

class MatchExpression {
#ifndef HAVE_PCRE
	std::string_view expression;
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

	[[gnu::pure]]
	bool operator()(std::string_view line) const noexcept;
};
