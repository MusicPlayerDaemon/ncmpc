// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TablePaint.hxx"
#include "TableLayout.hxx"
#include "TableStructure.hxx"
#include "strfsong.hxx"
#include "paint.hxx"
#include "util/LocaleString.hxx"

static void
FillSpace(const Window window, unsigned n) noexcept
{
	// TODO: use whline(), which unfortunately doesn't move the cursor
	while (n-- > 0)
		window.Char(' ');
}

void
PaintTableRow(const Window window, unsigned width,
	      bool selected, bool highlight, const struct mpd_song &song,
	      const TableLayout &layout) noexcept
{
	const auto color = highlight ? Style::LIST_BOLD : Style::LIST;
	row_color(window, color, selected);

	const size_t n_columns = layout.structure.columns.size();
	for (size_t i = 0; i < n_columns; ++i) {
		const auto &cl = layout.columns[i];
		const auto &cs = layout.structure.columns[i];
		if (cl.width == 0)
			break;

		if (i > 0) {
			SelectStyle(window, Style::LINE);
			window.Char(ACS_VLINE);
			row_color(window, color, selected);
		}

		char buffer[1024];

		std::string_view s = strfsong(buffer, cs.format.c_str(), &song);
		const char *end = AtWidthMB(s, cl.width);
		s = {s.data(), end};

		window.String(s);
		FillSpace(window, cl.width - StringWidthMB(s));
	}

	row_clear_to_eol(window, width, selected);
}
