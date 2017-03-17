/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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
struct hscroll {
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
	char *text;

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
};

static inline void
hscroll_reset(struct hscroll *hscroll)
{
	hscroll->offset = 0;
}

static inline void
hscroll_step(struct hscroll *hscroll)
{
	++hscroll->offset;
}

char *
strscroll(struct hscroll *hscroll, const char *str, const char *separator,
	  unsigned width);

/**
 * Initializes a #hscroll object allocated by the caller.
 */
static inline void
hscroll_init(struct hscroll *hscroll, WINDOW *w, const char *separator)
{
	hscroll->w = w;
	hscroll->separator = separator;
}

/**
 * Sets a text to scroll.  This installs a timer which redraws every
 * second with the current window attributes.  Call hscroll_clear() to
 * disable it.  This function automatically removes the old text.
 */
void
hscroll_set(struct hscroll *hscroll, unsigned x, unsigned y, unsigned width,
	    const char *text);

/**
 * Removes the text and the timer.  It may be reused with
 * hscroll_set().
 */
void
hscroll_clear(struct hscroll *hscroll);

/**
 * Explicitly draws the scrolled text.  Calling this function is only
 * allowed if there is a text currently.
 */
void
hscroll_draw(struct hscroll *hscroll);

#endif
