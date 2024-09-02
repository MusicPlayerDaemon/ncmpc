// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "SongRowPaint.hxx"
#include "strfsong.hxx"
#include "time_format.hxx"
#include "hscroll.hxx"
#include "Options.hxx"
#include "ui/Window.hxx"
#include "ui/paint.hxx"
#include "util/LocaleString.hxx"
#include "config.h" // IWYU pragma: keep

#include <mpd/client.h>

#include <string.h>

void
paint_song_row(const Window window, [[maybe_unused]] int y, unsigned width,
	       bool selected, bool highlight, const struct mpd_song &song,
	       [[maybe_unused]] class hscroll *hscroll, const char *format)
{
	char buffer[1024];

	const std::string_view text = strfsong(buffer, format, song);
	row_paint_text(window, width, highlight ? Style::LIST_BOLD : Style::LIST,
		       selected, text);

#ifndef NCMPC_MINI
	if (options.second_column && mpd_song_get_duration(&song) > 0) {
		char duration_buffer[32];
		const auto duration = format_duration_short(duration_buffer, mpd_song_get_duration(&song));
		width -= duration.size() + 1;
		window.MoveCursor({(int)width, y});
		window.Char(' ');
		window.String(duration);
	}

	if (hscroll != nullptr && width > 3 &&
	    StringWidthMB(text) >= width) {
		    hscroll->Set({0, y}, width, text,
			     highlight ? Style::LIST_BOLD : Style::LIST,
			     selected ? A_REVERSE : 0);
		hscroll->Paint();
	}
#endif
}
