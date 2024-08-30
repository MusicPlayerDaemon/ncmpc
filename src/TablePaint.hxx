// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

struct Window;
struct TableLayout;
struct mpd_song;

void
PaintTableRow(Window window, unsigned width,
	      bool selected, bool highlight, const struct mpd_song &song,
	      const TableLayout &layout) noexcept;
