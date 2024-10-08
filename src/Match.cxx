// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
MatchExpression::Compile(std::string_view src, bool anchor) noexcept
{
#ifndef HAVE_PCRE
	expression = src;
	anchored = anchor;

	return true;
#else
	assert(re == nullptr);

	int options = PCRE2_CASELESS|PCRE2_DOTALL|PCRE2_NO_AUTO_CAPTURE;
	if (anchor)
		options |= PCRE2_ANCHORED;

	int error_number;
	PCRE2_SIZE error_offset;
	re = pcre2_compile_8(PCRE2_SPTR8(src.data()), src.size(),
			     options, &error_number, &error_offset, nullptr);
	return re != nullptr;
#endif
}

bool
MatchExpression::operator()(std::string_view line) const noexcept
{
#ifndef HAVE_PCRE
	return anchored
		? strncasecmp(line.data(), expression.data(), std::min(expression.size(), line.size())) == 0
		: expression.find(line) != expression.npos;
#else
	assert(re != nullptr);

	const auto match_data =
		pcre2_match_data_create_from_pattern_8(re, nullptr);
	AtScopeExit(match_data) {
		pcre2_match_data_free_8(match_data);
	};

	return pcre2_match_8(re, (PCRE2_SPTR8)line.data(), line.size(),
			     0, 0, match_data, nullptr) >= 0;
#endif
}
