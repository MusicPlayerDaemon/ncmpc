// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TablePaint.hxx"
#include "TableLayout.hxx"
#include "TableStructure.hxx"
#include "strfsong.hxx"
#include "paint.hxx"
#include "util/LocaleString.hxx"

static void
FillSpace(WINDOW *w, unsigned n) noexcept
{
	// TODO: use whline(), which unfortunately doesn't move the cursor
	while (n-- > 0)
		waddch(w, ' ');
}

void
PaintTableRow(WINDOW *w, unsigned width,
	      bool selected, bool highlight, const struct mpd_song &song,
	      const TableLayout &layout) noexcept
{
	const auto color = highlight ? Style::LIST_BOLD : Style::LIST;
	row_color(w, color, selected);

	const size_t n_columns = layout.structure.columns.size();
	for (size_t i = 0; i < n_columns; ++i) {
		const auto &cl = layout.columns[i];
		const auto &cs = layout.structure.columns[i];
		if (cl.width == 0)
			break;

		if (i > 0) {
			SelectStyle(w, Style::LINE);
			waddch(w, ACS_VLINE);
			row_color(w, color, selected);
		}

		char buffer[1024];
		size_t length = strfsong(buffer, sizeof(buffer),
					 cs.format.c_str(), &song);

		const char *end = AtWidthMB(buffer, length, cl.width);
		length = end - buffer;

		waddnstr(w, buffer, length);
		FillSpace(w, cl.width - StringWidthMB(buffer, length));
	}

	row_clear_to_eol(w, width, selected);
}
