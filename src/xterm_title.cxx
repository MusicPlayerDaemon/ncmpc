// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "xterm_title.hxx"
#include "util/CharUtil.hxx"

#include <fmt/core.h>

#include <term.h>

#include <algorithm> // for std::all_of()

#include <stdio.h>
#include <stdlib.h>

using std::string_view_literals::operator""sv;

static const char *to_status_line_, *from_status_line_;

void
InitXtermTitle() noexcept
{
	to_status_line_ = tigetstr("tsl");
	from_status_line_ = tigetstr("fsl");

	if (to_status_line_ == (const char *)-1 ||
	    from_status_line_ == NULL ||
	    from_status_line_ == (const char *)-1)
		to_status_line_ = NULL;
}

[[gnu::const]]
static bool
SupportsXtermTitle() noexcept
{
	return to_status_line_ != nullptr;
}

[[gnu::pure]]
static bool
IsSafe(std::string_view s) noexcept
{
	return std::all_of(s.begin(), s.end(), [](char ch){
		return !IsNonPrintableASCII(ch);
	});
}

void
set_xterm_title(std::string_view title) noexcept
{
	if (!SupportsXtermTitle())
		return;

	if (!IsSafe(title))
		/* refuse to write strings with control characters to
		   the terminal as this may inject control
		   sequences */
		return;

	fmt::print("{}{}{}"sv, to_status_line_, title, from_status_line_);
	fflush(stdout);
}
