// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Completion.hxx"

#include <assert.h>

Completion::Result
Completion::Complete(const std::string &prefix) const noexcept
{
	auto lower = list.lower_bound(prefix);
	if (lower == list.end() || !lower->starts_with(prefix))
		return {std::string(), {lower, lower}};

	auto upper = list.upper_bound(prefix);
	while (upper != list.end() && upper->starts_with(prefix))
		++upper;

	assert(upper != lower);

	auto m = std::mismatch(lower->begin(), lower->end(),
			       std::prev(upper)->begin()).first;

	return {{lower->begin(), m}, {lower, upper}};
}
