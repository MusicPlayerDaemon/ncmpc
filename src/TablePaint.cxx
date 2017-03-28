/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
