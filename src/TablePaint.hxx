// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <curses.h>

struct TableLayout;
struct mpd_song;

void
PaintTableRow(WINDOW *w, unsigned width,
	      bool selected, bool highlight, const struct mpd_song &song,
	      const TableLayout &layout) noexcept;
