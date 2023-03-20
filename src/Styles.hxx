// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef STYLES_HXX
#define STYLES_HXX

#include "config.h"

#include <curses.h>

enum class Style : unsigned {
	/**
	 * The ncurses default style.
	 *
	 * @see assume_default_colors(3ncurses)
	 */
	DEFAULT,

	TITLE,
	TITLE_BOLD,
	LINE,
	LINE_BOLD,
	LINE_FLAGS,
	LIST,
	LIST_BOLD,
	PROGRESSBAR,
	PROGRESSBAR_BACKGROUND,
	STATUS,
	STATUS_BOLD,
	STATUS_TIME,
	STATUS_ALERT,
	DIRECTORY,
	PLAYLIST,
	BACKGROUND,
	END
};

#ifdef ENABLE_COLORS

/**
 * Throws on error.
 */
void
ModifyStyle(const char *name, const char *value);

void
ApplyStyles() noexcept;

#endif

void
SelectStyle(WINDOW *w, Style style) noexcept;

#endif
