// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Completion.hxx"

#include <assert.h>

static bool
StartsWith(const std::string &haystack, const std::string &needle) noexcept
{
	return haystack.length() >= needle.length() &&
		std::equal(needle.begin(), needle.end(), haystack.begin());
}

Completion::Result
Completion::Complete(const std::string &prefix) const noexcept
{
	auto lower = list.lower_bound(prefix);
	if (lower == list.end() || !StartsWith(*lower, prefix))
		return {std::string(), {lower, lower}};

	auto upper = list.upper_bound(prefix);
	while (upper != list.end() && StartsWith(*upper, prefix))
		++upper;

	assert(upper != lower);

	auto m = std::mismatch(lower->begin(), lower->end(),
			       std::prev(upper)->begin()).first;

	return {{lower->begin(), m}, {lower, upper}};
}
