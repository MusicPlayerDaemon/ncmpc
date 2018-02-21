/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#ifndef HSCROLL_H
#define HSCROLL_H

#include "config.h"
#include "ncmpc_curses.h"

#include <glib.h>

/**
 * This class is used to auto-scroll text which does not fit on the
 * screen.  Call hscroll_init() to initialize the object,
 * hscroll_clear() to free resources, and hscroll_set() to begin
 * scrolling.
 */
class hscroll {
	WINDOW *w;
	const char *separator;

	/**
	 * The bounding coordinates on the screen.
	 */
	unsigned x, y, width;

	/**
	 * ncurses attributes for drawing the text.
	 */
	attr_t attrs;

	/**
	 * ncurses colors for drawing the text.
	 */
	short pair;

	/**
	 * The scrolled text, in the current locale.
	 */
	char *text = nullptr;

	/**
	 * The current scrolling offset.  This is a character
	 * position, not a screen column.
	 */
	gsize offset;

	/**
	 * The id of the timer which updates the scrolled area every
	 * second.
	 */
	guint source_id;

public:
	void Init(WINDOW *_w, const char *_separator) {
		w = _w;
		separator = _separator;
	}

	/**
	 * Sets a text to scroll.  This installs a timer which redraws
	 * every second with the current window attributes.  Call
	 * hscroll_clear() to disable it.
	 */
	void Set(unsigned x, unsigned y, unsigned width, const char *text);

	/**
	 * Removes the text and the timer.  It may be reused with
	 * Set().
	 */
	void Clear();

	void Rewind() {
		offset = 0;
	}

	void Step() {
		++offset;
	}

	char *ScrollString(const char *str, const char *separator,
			   unsigned width);

	/**
	 * Explicitly draws the scrolled text.  Calling this function
	 * is only allowed if there is a text currently.
	 */
	void Paint();

private:
	static gboolean TimerCallback(gpointer data);
	void TimerCallback();
};

#endif
