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

#ifndef LIST_WINDOW_H
#define LIST_WINDOW_H

#include "config.h"
#include "command.hxx"
#include "colors.hxx"
#include "ncmpc_curses.h"

#include <glib.h>

typedef const char *
(*list_window_callback_fn_t)(unsigned i, void *data);

typedef void
(*list_window_paint_callback_t)(WINDOW *w, unsigned i,
				unsigned y, unsigned width,
				bool selected,
				const void *data);

struct ListWindow {
	WINDOW *w;
	unsigned rows, cols;

	/**
	 * Number of items in this list.
	 */
	unsigned length;

	unsigned start;
	unsigned selected;

	/**
	 * Represents the base item.
	 */
	unsigned range_base;

	/**
	 * Range selection activated?
	 */
	bool range_selection;

	bool hide_cursor;
};

/**
 * The bounds of a range selection, see list_window_get_range().
 */
struct ListWindowRange {
	/**
	 * The index of the first selected item.
	 */
	unsigned start;

	/**
	 * The index after the last selected item.  The selection is
	 * empty when this is the same as "start".
	 */
	unsigned end;
};

/* create a new list window */
ListWindow *
list_window_init(WINDOW *w, unsigned width, unsigned height);

/* destroy a list window */
void list_window_free(ListWindow *lw);

/* reset a list window (selected=0, start=0) */
void list_window_reset(ListWindow *lw);

void
list_window_resize(ListWindow *lw, unsigned width, unsigned height);

void
list_window_set_length(ListWindow *lw, unsigned length);

/* paint a list window */
void list_window_paint(const ListWindow *lw,
		       list_window_callback_fn_t callback,
		       void *callback_data);

void
list_window_paint2(const ListWindow *lw,
		   list_window_paint_callback_t paint_callback,
		   const void *callback_data);

/* perform basic list window commands (movement) */
bool
list_window_cmd(ListWindow *lw, command_t cmd);

/**
 * Scroll the window.  Returns true if the command has been
 * consumed.
 */
bool
list_window_scroll_cmd(ListWindow *lw, command_t cmd);

#ifdef HAVE_GETMOUSE
/**
 * The mouse was clicked.  Check if the list should be scrolled
 * Returns non-zero if the mouse event has been handled.
 */
bool
list_window_mouse(ListWindow *lw, unsigned long bstate, int y);
#endif

/**
 * Centers the visible range around item n on the list.
 */
void
list_window_center(ListWindow *lw, unsigned n);

/**
 * Scrolls the view to item n, as if the cursor would have been moved
 * to the position.
 */
void
list_window_scroll_to(ListWindow *lw, unsigned n);

/**
 * Sets the position of the cursor.  Disables range selection.
 */
void
list_window_set_cursor(ListWindow *lw, unsigned i);

/**
 * Moves the cursor.  Modifies the range if range selection is
 * enabled.
 */
void
list_window_move_cursor(ListWindow *lw, unsigned n);

/**
 * Ensures that the cursor is visible on the screen, i.e. it is not
 * outside the current scrolling range.
 */
void
list_window_fetch_cursor(ListWindow *lw);

/**
 * Determines the lower and upper bound of the range selection.  If
 * range selection is disabled, it returns the cursor position (range
 * length is 1).
 */
void
list_window_get_range(const ListWindow *lw,
		      ListWindowRange *range);

/* find a string in a list window */
bool
list_window_find(ListWindow *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str,
		 bool wrap,
		 bool bell_on_wrap);

/* find a string in a list window (reversed) */
bool
list_window_rfind(ListWindow *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  bool wrap,
		  bool bell_on_wrap);

/* find a string in a list window which begins with the given characters in *str */
bool
list_window_jump(ListWindow *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str);

#endif
