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
SelectRowStyle(const Window window, Style style, bool selected) noexcept
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
EndRowStyle(const Window window) noexcept
{
	window.AttributeOff(A_REVERSE);
}

/**
 * Clears the remaining space on the current row.  If the row is
 * selected and the wide_cursor option is enabled, it draws the cursor
 * on the space.
 */
void
ClearRowEnd(const Window window, unsigned width, bool selected) noexcept;

/**
 * Paint a plain-text row.
 */
void
PaintTextRow(const Window window, unsigned width,
	     Style style, bool selected,
	     std::string_view text) noexcept;

/**
 * Paint a row with a button triggering an action.
 */
void
PaintButtonRow(const Window window, unsigned width, bool selected,
	       std::string_view text) noexcept;
