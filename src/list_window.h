/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef LIST_WINDOW_H
#define LIST_WINDOW_H

#include "../config.h"
#include "command.h"

#include <glib.h>
#include <stdbool.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

typedef const char *(*list_window_callback_fn_t)(unsigned index,
						 bool *highlight,
						 void *data);

typedef struct list_window {
	WINDOW *w;
	unsigned rows, cols;

	unsigned start;
	unsigned selected;
	unsigned xoffset;

	bool hide_cursor;
} list_window_t;


/* create a new list window */
struct list_window *list_window_init(WINDOW *w,
				     unsigned width, unsigned height);

/* destroy a list window (returns NULL) */
void list_window_free(struct list_window *lw);

/* reset a list window (selected=0, start=0) */
void list_window_reset(struct list_window *lw);

/* paint a list window */
void list_window_paint(struct list_window *lw,
		       list_window_callback_fn_t callback,
		       void *callback_data);

/* perform basic list window commands (movement) */
bool
list_window_cmd(struct list_window *lw, unsigned rows, command_t cmd);

/**
 * Scroll the window.  Returns non-zero if the command has been
 * consumed.
 */
bool
list_window_scroll_cmd(struct list_window *lw, unsigned rows, command_t cmd);

#ifdef HAVE_GETMOUSE
/**
 * The mouse was clicked.  Check if the list should be scrolled
 * Returns non-zero if the mouse event has been handled.
 */
bool
list_window_mouse(struct list_window *lw, unsigned rows,
		  unsigned long bstate, int y);
#endif

void
list_window_center(struct list_window *lw, unsigned rows, unsigned n);

/* select functions */
void list_window_set_selected(struct list_window *lw, unsigned n);
void list_window_check_selected(struct list_window *lw, unsigned length);

/* find a string in a list window */
bool
list_window_find(struct list_window *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str,
		 bool wrap);

/* find a string in a list window (reversed) */
bool
list_window_rfind(struct list_window *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  bool wrap,
		  unsigned rows);

#endif
