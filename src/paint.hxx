// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Styles.hxx"
#include "Options.hxx"
#include "Window.hxx"

/**
 * Sets the specified color, and enables "reverse" mode if selected is
 * true.
 */
static inline void
row_color(const Window window, Style style, bool selected) noexcept
{
	SelectStyle(window, style);

	if (selected)
		window.AttributeOn(A_REVERSE);
	else
		window.AttributeOff(A_REVERSE);
}

/**
 * Call this when you are done with painting rows.  It resets the
 * "reverse" mode.
 */
static inline void
row_color_end(const Window window) noexcept
{
	window.AttributeOff(A_REVERSE);
}

/**
 * Clears the remaining space on the current row.  If the row is
 * selected and the wide_cursor option is enabled, it draws the cursor
 * on the space.
 */
static inline void
row_clear_to_eol(const Window window, unsigned width, bool selected) noexcept
{
	if (selected && options.wide_cursor)
		window.HLine(width, ' ');
	else
		window.ClearToEol();
}

/**
 * Paint a plain-text row.
 */
static inline void
row_paint_text(const Window window, unsigned width,
	       Style style, bool selected,
	       const char *text) noexcept
{
	row_color(window, style, selected);

	window.String(text);

	/* erase the unused space after the text */
	row_clear_to_eol(window, width, selected);
}
