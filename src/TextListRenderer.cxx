/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "TextListRenderer.hxx"
#include "ListText.hxx"
#include "paint.hxx"

#include <assert.h>

static void
list_window_paint_row(WINDOW *w, unsigned width, bool selected,
		      const char *text) noexcept
{
	row_paint_text(w, width, Style::LIST,
		       selected, text);
}

void
TextListRenderer::PaintListItem(WINDOW *w, unsigned i, unsigned,
				unsigned width, bool selected) const noexcept
{
	char buffer[1024];
	const char *label = text.GetListItemText(buffer, sizeof(buffer), i);
	assert(label != nullptr);

	list_window_paint_row(w, width, selected, label);
}
