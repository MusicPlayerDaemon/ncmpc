// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Bell.hxx"
#include "Options.hxx"

#include <curses.h>

void
Bell() noexcept
{
	if (ui_options.audible_bell)
		beep();
	if (ui_options.visible_bell)
		flash();
}
