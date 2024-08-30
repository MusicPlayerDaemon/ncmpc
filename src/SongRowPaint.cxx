// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "SongRowPaint.hxx"
#include "Window.hxx"
#include "paint.hxx"
#include "strfsong.hxx"
#include "time_format.hxx"
#include "hscroll.hxx"
#include "config.h" // IWYU pragma: keep
#include "util/LocaleString.hxx"

#include <mpd/client.h>

#include <string.h>

void
paint_song_row(const Window window, [[maybe_unused]] int y, unsigned width,
	       bool selected, bool highlight, const struct mpd_song *song,
	       [[maybe_unused]] class hscroll *hscroll, const char *format)
{
	char buffer[1024];

	strfsong(buffer, sizeof(buffer), format, song);
	row_paint_text(window, width, highlight ? Style::LIST_BOLD : Style::LIST,
		       selected, buffer);

#ifndef NCMPC_MINI
	if (options.second_column && mpd_song_get_duration(song) > 0) {
		char duration[32];
		format_duration_short(duration, sizeof(duration),
				      mpd_song_get_duration(song));
		width -= strlen(duration) + 1;
		window.MoveCursor({(int)width, y});
		window.Char(' ');
		window.String(duration);
	}

	if (hscroll != nullptr && width > 3 &&
	    StringWidthMB(buffer) >= width) {
		    hscroll->Set({0, y}, width, buffer,
			     highlight ? Style::LIST_BOLD : Style::LIST,
			     selected ? A_REVERSE : 0);
		hscroll->Paint();
	}
#endif
}
