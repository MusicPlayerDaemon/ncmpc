// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "xterm_title.hxx"
#include "Options.hxx"

#include <stdio.h>
#include <stdlib.h>

void
set_xterm_title(const char *title) noexcept
{
	/* the current xterm title exists under the WM_NAME property */
	/* and can be retrieved with xprop -id $WINDOWID */

	if (!options.enable_xterm_title)
		return;

	if (getenv("WINDOWID") == nullptr) {
		options.enable_xterm_title = false;
		return;
	}

	printf("\033]0;%s\033\\", title);
	fflush(stdout);
}
