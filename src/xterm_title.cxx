// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "xterm_title.hxx"
#include "Options.hxx"

#include <fmt/core.h>

#include <term.h>

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

void
set_xterm_title(std::string_view title) noexcept
{
	if (!options.enable_xterm_title || !SupportsXtermTitle())
		return;

	fmt::print("{}{}{}"sv, to_status_line_, title, from_status_line_);
	fflush(stdout);
}
