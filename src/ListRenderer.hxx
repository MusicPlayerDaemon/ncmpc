// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef LIST_RENDERER_HXX
#define LIST_RENDERER_HXX

#include <curses.h>

class ListRenderer {
public:
	virtual void PaintListItem(WINDOW *w, unsigned i,
				   unsigned y, unsigned width,
				   bool selected) const noexcept = 0;
};

#endif
