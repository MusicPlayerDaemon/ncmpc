// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_TAB_BAR_HXX
#define NCMPC_TAB_BAR_HXX

#include <curses.h>

struct PageMeta;

void
PaintTabBar(WINDOW *w, const PageMeta &current_page_meta,
	    const char *current_page_title) noexcept;

#endif
