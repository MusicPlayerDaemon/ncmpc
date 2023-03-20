// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_PAINT_H
#define NCMPC_PAINT_H

#include "Styles.hxx"
#include "Options.hxx"

/**
 * Sets the specified color, and enables "reverse" mode if selected is
 * true.
 */
static inline void
row_color(WINDOW *w, Style style, bool selected) noexcept
{
	SelectStyle(w, style);

	if (selected)
		wattron(w, A_REVERSE);
	else
		wattroff(w, A_REVERSE);
}

/**
 * Call this when you are done with painting rows.  It resets the
 * "reverse" mode.
 */
static inline void
row_color_end(WINDOW *w) noexcept
{
	wattroff(w, A_REVERSE);
}

/**
 * Clears the remaining space on the current row.  If the row is
 * selected and the wide_cursor option is enabled, it draws the cursor
 * on the space.
 */
static inline void
row_clear_to_eol(WINDOW *w, unsigned width, bool selected) noexcept
{
	if (selected && options.wide_cursor)
		whline(w, ' ', width);
	else
		wclrtoeol(w);
}

/**
 * Paint a plain-text row.
 */
static inline void
row_paint_text(WINDOW *w, unsigned width,
	       Style style, bool selected,
	       const char *text) noexcept
{
	row_color(w, style, selected);

	waddstr(w, text);

	/* erase the unused space after the text */
	row_clear_to_eol(w, width, selected);
}

#endif
