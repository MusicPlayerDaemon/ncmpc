/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "list_window.h"
#include "config.h"
#include "options.h"
#include "charset.h"
#include "support.h"
#include "command.h"
#include "colors.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern void screen_bell(void);

struct list_window *
list_window_init(WINDOW *w, unsigned width, unsigned height)
{
	struct list_window *lw;

	lw = g_malloc0(sizeof(list_window_t));
	lw->w = w;
	lw->cols = width;
	lw->rows = height;
	return lw;
}

void
list_window_free(struct list_window *lw)
{
	if (lw) {
		memset(lw, 0, sizeof(list_window_t));
		g_free(lw);
	}
}

void
list_window_reset(struct list_window *lw)
{
	lw->selected = 0;
	lw->xoffset = 0;
	lw->start = 0;
}

void
list_window_check_selected(struct list_window *lw, unsigned length)
{
	if (lw->start + lw->rows > length) {
		if (length > lw->rows)
			lw->start = length - lw->rows;
		else
			lw->start = 0;
	}

	if (lw->selected < lw->start)
		lw->selected = lw->start;

	if (length > 0 && lw->selected >= length)
		lw->selected = length - 1;
}

void
list_window_center(struct list_window *lw, unsigned rows, unsigned n)
{
	if (n > lw->rows / 2)
		lw->start = n - lw->rows / 2;
	else
		lw->start = 0;

	if (lw->start + lw->rows > rows) {
		if (lw->rows < rows)
			lw->start = rows - lw->rows;
		else
			lw->start = 0;
	}
}

void
list_window_set_selected(struct list_window *lw, unsigned n)
{
	lw->selected = n;
}

static void
list_window_next(struct list_window *lw, unsigned length)
{
	if (lw->selected + 1 < length)
		lw->selected++;
	else if (options.list_wrap)
		lw->selected = 0;
}

static void
list_window_previous(struct list_window *lw, unsigned length)
{
	if (lw->selected > 0)
		lw->selected--;
	else if (options.list_wrap)
		lw->selected = length - 1;
}

static void
list_window_first(struct list_window *lw)
{
	lw->xoffset = 0;
	lw->selected = 0;
}

static void
list_window_last(struct list_window *lw, unsigned length)
{
	lw->xoffset = 0;
	if (length > 0)
		lw->selected = length - 1;
	else
		lw->selected = 0;
}

static void
list_window_next_page(struct list_window *lw, unsigned length)
{
	if (lw->rows < 2)
		return;
	if (lw->selected + lw->rows < length)
		lw->selected += lw->rows - 1;
	else
		list_window_last(lw, length);
}

static void
list_window_previous_page(struct list_window *lw)
{
	if (lw->rows < 2)
		return;
	if (lw->selected > lw->rows - 1)
		lw->selected -= lw->rows - 1;
	else
		list_window_first(lw);
}


void
list_window_paint(struct list_window *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data)
{
	unsigned i;
	int fill = options.wide_cursor;
	int show_cursor = !(lw->flags & LW_HIDE_CURSOR);

	if (show_cursor) {
		if (lw->selected < lw->start)
			lw->start = lw->selected;

		if (lw->selected >= lw->start + lw->rows)
			lw->start = lw->selected - lw->rows + 1;
	}

	for (i = 0; i < lw->rows; i++) {
		int highlight = 0;
		const char *label;

		label = callback(lw->start + i, &highlight, callback_data);
		wmove(lw->w, i, 0);

		if (label) {
			int selected = lw->start + i == lw->selected;
			size_t len = my_strlen(label);

			if (highlight)
				colors_use(lw->w, COLOR_LIST_BOLD);
			else
				colors_use(lw->w, COLOR_LIST);

			if (show_cursor && selected)
				wattron(lw->w, A_REVERSE);

			//waddnstr(lw->w, label, lw->cols);
			waddstr(lw->w, label);
			if (fill && len < lw->cols)
				whline(lw->w,  ' ', lw->cols-len);

			if (selected)
				wattroff(lw->w, A_REVERSE);

			if (!fill && len < lw->cols)
				wclrtoeol(lw->w);
		} else
			wclrtoeol(lw->w);
	}
}

int
list_window_find(struct list_window *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str,
		 int wrap)
{
	int h;
	unsigned i = lw->selected + 1;
	const char *label;

	while (wrap || i == lw->selected + 1) {
		while ((label = callback(i,&h,callback_data))) {
			if (str && label && strcasestr(label, str)) {
				lw->selected = i;
				return 0;
			}
			if (wrap && i == lw->selected)
				return 1;
			i++;
		}
		if (wrap) {
			if (i == 0) /* empty list */
				return 1;
			i=0; /* first item */
			screen_bell();
		}
	}

	return 1;
}

int
list_window_rfind(struct list_window *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  int wrap,
		  unsigned rows)
{
	int h;
	int i = lw->selected - 1;
	const char *label;

	if (rows == 0)
		return 1;

	while (wrap || i == (int)lw->selected - 1) {
		while (i >= 0 && (label = callback(i,&h,callback_data))) {
			if( str && label && strcasestr(label, str) ) {
				lw->selected = i;
				return 0;
			}
			if (wrap && i == (int)lw->selected)
				return 1;
			i--;
		}
		if (wrap) {
			i = rows - 1; /* last item */
			screen_bell();
		}
	}
	return 1;
}

/* perform basic list window commands (movement) */
int
list_window_cmd(struct list_window *lw, unsigned rows, command_t cmd)
{
	switch (cmd) {
	case CMD_LIST_PREVIOUS:
		list_window_previous(lw, rows);
		break;
	case CMD_LIST_NEXT:
		list_window_next(lw, rows);
		break;
	case CMD_LIST_FIRST:
		list_window_first(lw);
		break;
	case CMD_LIST_LAST:
		list_window_last(lw, rows);
		break;
	case CMD_LIST_NEXT_PAGE:
		list_window_next_page(lw, rows);
		break;
	case CMD_LIST_PREVIOUS_PAGE:
		list_window_previous_page(lw);
		break;
	default:
		return 0;
	}

	return 1;
}

int
list_window_scroll_cmd(struct list_window *lw, unsigned rows, command_t cmd)
{
	switch (cmd) {
	case CMD_LIST_PREVIOUS:
		if (lw->start > 0)
			lw->start--;
		break;

	case CMD_LIST_NEXT:
		if (lw->start + lw->rows < rows)
			lw->start++;
		break;

	case CMD_LIST_FIRST:
		lw->start = 0;
		break;

	case CMD_LIST_LAST:
		if (rows > lw->rows)
			lw->start = rows - lw->rows;
		else
			lw->start = 0;
		break;

	case CMD_LIST_NEXT_PAGE:
		lw->start += lw->rows - 1;
		if (lw->start + lw->rows > rows) {
			if (rows > lw->rows)
				lw->start = rows - lw->rows;
			else
				lw->start = 0;
		}
		break;

	case CMD_LIST_PREVIOUS_PAGE:
		if (lw->start > lw->rows)
			lw->start -= lw->rows;
		else
			lw->start = 0;
		break;

	default:
		return 0;
	}

	return 1;
}

#ifdef HAVE_GETMOUSE
int
list_window_mouse(struct list_window *lw, unsigned rows,
		  unsigned long bstate, int y)
{
	assert(lw != NULL);

	/* if the even occured above the list window move up */
	if (y < 0) {
		if (bstate & BUTTON3_CLICKED)
			list_window_first(lw);
		else
			list_window_previous_page(lw);
		return 1;
	}

	/* if the even occured below the list window move down */
	if ((unsigned)y >= rows) {
		if (bstate & BUTTON3_CLICKED)
			list_window_last(lw, rows);
		else
			list_window_next_page(lw, rows);
		return 1;
	}

	return 0;
}
#endif

list_window_state_t *
list_window_init_state(void)
{
	return g_malloc0(sizeof(list_window_state_t));
}

void
list_window_free_state(list_window_state_t *state)
{
	if (state) {
		if (state->list) {
			GList *list = state->list;

			while (list) {
				g_free(list->data);
				list->data = NULL;
				list = list->next;
			}

			g_list_free(state->list);
			state->list = NULL;
		}

		g_free(state);
	}
}

void
list_window_push_state(list_window_state_t *state, struct list_window *lw)
{
	if (state) {
		struct list_window *tmp = g_malloc(sizeof(list_window_t));
		memcpy(tmp, lw, sizeof(list_window_t));
		state->list = g_list_prepend(state->list, (gpointer) tmp);
		list_window_reset(lw);
	}
}

bool
list_window_pop_state(list_window_state_t *state, struct list_window *lw)
{
	if (state && state->list) {
		struct list_window *tmp = state->list->data;

		memcpy(lw, tmp, sizeof(list_window_t));
		g_free(tmp);
		state->list->data = NULL;
		state->list = g_list_delete_link(state->list, state->list);
	}

	// return TRUE if there are still states in the list
	return (state && state->list) ? TRUE : FALSE;
}
