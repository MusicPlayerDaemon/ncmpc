// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "RowPaint.hxx"
#include "Options.hxx"

void
ClearRowEnd(const Window window, unsigned width, bool selected) noexcept
{
	if (selected && ui_options.wide_cursor)
		window.HLine(width, ' ');
	else
		window.ClearToEol();
}

void
PaintTextRow(const Window window, unsigned width,
	     Style style, bool selected,
	     std::string_view text) noexcept
{
	SelectRowStyle(window, style, selected);

	window.String(text);

	/* erase the unused space after the text */
	ClearRowEnd(window, width, selected);
}

void
PaintButtonRow(const Window window, unsigned width, bool selected,
	       std::string_view text) noexcept
{
	PaintTextRow(window, width, Style::DIRECTORY, selected, text);
}
