// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "xterm_title.hxx"
#include "Options.hxx"

#include <fmt/core.h>

#include <stdio.h>
#include <stdlib.h>

using std::string_view_literals::operator""sv;

void
set_xterm_title(std::string_view title) noexcept
{
	/* the current xterm title exists under the WM_NAME property */
	/* and can be retrieved with xprop -id $WINDOWID */

	if (!options.enable_xterm_title)
		return;

	if (getenv("WINDOWID") == nullptr) {
		options.enable_xterm_title = false;
		return;
	}

	fmt::print("\033]0;{}\033\\"sv, title);
	fflush(stdout);
}
