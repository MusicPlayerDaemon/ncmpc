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

#include "list_window.h"
#include "config.h"
#include "options.h"
#include "charset.h"
#include "match.h"
#include "command.h"
#include "colors.h"
#include "screen.h"
#include "i18n.h"

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
	lw->visual_selection = false;
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
	lw->selected_start = 0;
	lw->selected_end = 0;
	lw->visual_selection = false;
	lw->visual_base = 0;
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

	if (length == 0)
		lw->selected = 0;
	else if (lw->selected >= length)
		lw->selected = length - 1;

	if(lw->visual_selection)
	{
		if(lw->visual_base > lw->selected_end)
			  lw->selected_end = lw->selected;
		if(lw->visual_base < lw->selected_start)
			  lw->selected_start = lw->selected;
	}
	else
	{
		lw->selected_start = lw->selected;
		lw->selected_end = lw->selected;
	}
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
	if(lw->visual_selection)
	{
		if(n >= lw->visual_base)
		{
			lw->selected_end = n;
			lw->selected_start = lw->visual_base;
		}
		if(n <= lw->visual_base)
		{
			lw->selected_start = n;
			lw->selected_end = lw->visual_base;
		}
	}
	else
	{
		lw->selected_start = n;
		lw->selected_end = n;
	}
}

static void
list_window_next(struct list_window *lw, unsigned length)
{
	if (lw->selected + 1 < length)
		list_window_set_selected(lw, lw->selected + 1);
	else if (options.list_wrap)
		list_window_set_selected(lw, 0);
}

static void
list_window_previous(struct list_window *lw, unsigned length)
{
	if (lw->selected > 0)
		list_window_set_selected(lw, lw->selected - 1);
	else if (options.list_wrap)
		list_window_set_selected(lw, length-1);
}

static void
list_window_top(struct list_window *lw)
{
	list_window_set_selected(lw, lw->start);
}

static void
list_window_middle(struct list_window *lw, unsigned length)
{
	if (length >= lw->rows)
		list_window_set_selected(lw, lw->start + lw->rows / 2);
	else
		list_window_set_selected(lw, length / 2);
}

static void
list_window_bottom(struct list_window *lw, unsigned length)
{
	if (length >= lw->rows)
		list_window_set_selected(lw, lw->start + lw->rows - 1);
	else
		list_window_set_selected(lw, length - 1);
}

static void
list_window_first(struct list_window *lw)
{
	lw->xoffset = 0;
	list_window_set_selected(lw, 0);
}

static void
list_window_last(struct list_window *lw, unsigned length)
{
	lw->xoffset = 0;
	if (length > 0)
		list_window_set_selected(lw, length - 1);
	else
		list_window_set_selected(lw, 0);
}

static void
list_window_next_page(struct list_window *lw, unsigned length)
{
	if (lw->rows < 2)
		return;
	if (lw->selected + lw->rows < length)
		list_window_set_selected(lw, lw->selected + lw->rows - 1);
	else
		list_window_last(lw, length);
}

static void
list_window_previous_page(struct list_window *lw)
{
	if (lw->rows < 2)
		return;
	if (lw->selected > lw->rows - 1)
		list_window_set_selected(lw, lw->selected - lw->rows + 1);
	else
		list_window_first(lw);
}

static void
list_window_scroll_up(struct list_window *lw, unsigned n)
{
	if (lw->start > 0) {
		if (n > lw->start)
			lw->start = 0;
		else
			lw->start -= n;
		if (lw->selected > lw->start + lw->rows - 1) {
			lw->selected = lw->start + lw->rows - 1;
			if (lw->visual_selection) {
				if (lw->selected < lw->visual_base) {
					lw->selected_start = lw->selected;
					lw->selected_end = lw->visual_base;
				} else {
					lw->selected_end = lw->selected;
				}
			} else {
				lw->selected_start = lw->selected;
				lw->selected_end = lw->selected;
			}
		}
	}
}

static void
list_window_scroll_down(struct list_window *lw, unsigned length, unsigned n)
{
	if (lw->start + lw->rows < length - 1)
	{
		if ( lw->start + lw->rows + n > length - 1)
			lw->start = length-1;
		else
			lw->start += n;
		if (lw->selected < lw->start) {
			lw->selected = lw->start;
			if (lw->visual_selection) {
				if (lw->selected > lw->visual_base) {
					lw->selected_end = lw->selected;
					lw->selected_start = lw->visual_base;
				} else {
					lw->selected_start = lw->selected;
				}
			} else {
				lw->selected_start = lw->selected;
				lw->selected_end = lw->selected;
			}
		}
	}
}

void
list_window_paint(struct list_window *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data)
{
	unsigned i;
	bool fill = options.wide_cursor;
	bool show_cursor = !lw->hide_cursor;

	if (show_cursor) {
		if (lw->selected < lw->start)
			lw->start = lw->selected;

		if (lw->selected >= lw->start + lw->rows)
			lw->start = lw->selected - lw->rows + 1;
	}

	for (i = 0; i < lw->rows; i++) {
		bool highlight = false;
		const char *label;

		label = callback(lw->start + i, &highlight, callback_data);
		wmove(lw->w, i, 0);

		if (label) {
			bool selected = (lw->start + i >= lw->selected_start && lw->start + i <= lw->selected_end);
			unsigned len = utf8_width(label);

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

bool
list_window_find(struct list_window *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str,
		 bool wrap,
		 bool bell_on_wrap)
{
	bool h;
	unsigned i = lw->selected + 1;
	const char *label;

	do {
		while ((label = callback(i,&h,callback_data))) {
			if (str && label && match_line(label, str)) {
				lw->selected = i;
				if(!lw->visual_selection || i > lw->selected_end)
					  lw->selected_end = i;
				if(!lw->visual_selection || i < lw->selected_start)
					  lw->selected_start = i;
				return true;
			}
			if (wrap && i == lw->selected)
				return false;
			i++;
		}
		if (wrap) {
			if (i == 0) /* empty list */
				return 1;
			i=0; /* first item */
			if (bell_on_wrap) {
				screen_bell();
			}
		}
	} while (wrap);

	return false;
}

bool
list_window_rfind(struct list_window *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  bool wrap,
		  bool bell_on_wrap,
		  unsigned rows)
{
	bool h;
	int i = lw->selected - 1;
	const char *label;

	if (rows == 0)
		return false;

	do {
		while (i >= 0 && (label = callback(i,&h,callback_data))) {
			if( str && label && match_line(label, str) ) {
				lw->selected = i;
				if(!lw->visual_selection || i > (int)lw->selected_end)
					  lw->selected_end = i;
				if(!lw->visual_selection || i < (int)lw->selected_start)
					  lw->selected_start = i;
				return true;
			}
			if (wrap && i == (int)lw->selected)
				return false;
			i--;
		}
		if (wrap) {
			i = rows - 1; /* last item */
			if (bell_on_wrap) {
				screen_bell();
			}
		}
	} while (wrap);

	return false;
}

bool
list_window_jump(struct list_window *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str)
{
	bool h;
	unsigned i = 0;
	const char *label;

	while ((label = callback(i,&h,callback_data))) {
		if (label && label[0] != '[')
		{
			if (str && label && g_ascii_strncasecmp(label, str, strlen(str)) == 0) {
				lw->selected = i;
				if(!lw->visual_selection || i > lw->selected_end)
					  lw->selected_end = i;
				if(!lw->visual_selection || i < lw->selected_start)
					  lw->selected_start = i;
				return true;
			}
		}
		else if (str && label && g_ascii_strncasecmp(label+1, str, strlen(str)) == 0) {
				lw->selected = i;
				if(!lw->visual_selection || i > lw->selected_end)
					  lw->selected_end = i;
				if(!lw->visual_selection || i < lw->selected_start)
					  lw->selected_start = i;
				return true;
			}
		i++;
	}
	return false;
}

/* perform basic list window commands (movement) */
bool
list_window_cmd(struct list_window *lw, unsigned rows, command_t cmd)
{
	switch (cmd) {
	case CMD_LIST_PREVIOUS:
		list_window_previous(lw, rows);
		break;
	case CMD_LIST_NEXT:
		list_window_next(lw, rows);
		break;
	case CMD_LIST_TOP:
		list_window_top(lw);
		break;
	case CMD_LIST_MIDDLE:
		list_window_middle(lw,rows);
		break;
	case CMD_LIST_BOTTOM:
		list_window_bottom(lw,rows);
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
	case CMD_LIST_VISUAL_SELECT:
		if(lw->visual_selection)
		{
			screen_status_printf(_("Visual selection disabled"));
			lw->visual_selection = false;
			list_window_set_selected(lw, lw->selected);
		}
		else
		{
			screen_status_printf(_("Visual selection enabled"));
			lw->visual_base = lw->selected;
			lw->visual_selection = true;
		}
		break;
	case CMD_LIST_SCROLL_UP_LINE:
		list_window_scroll_up(lw, 1);
		break;
	case CMD_LIST_SCROLL_DOWN_LINE:
		list_window_scroll_down(lw, rows, 1);
		break;
	case CMD_LIST_SCROLL_UP_HALF:
		list_window_scroll_up(lw, (lw->rows - 1) / 2);
		break;
	case CMD_LIST_SCROLL_DOWN_HALF:
		list_window_scroll_down(lw, rows, (lw->rows - 1) / 2);
		break;
	default:
		return false;
	}

	return true;
}

bool
list_window_scroll_cmd(struct list_window *lw, unsigned rows, command_t cmd)
{
	switch (cmd) {
	case CMD_LIST_SCROLL_UP_LINE:
	case CMD_LIST_PREVIOUS:
		if (lw->start > 0)
			lw->start--;
		break;

	case CMD_LIST_SCROLL_DOWN_LINE:
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

	case CMD_LIST_SCROLL_UP_HALF:
		if (lw->start > (lw->rows - 1) / 2)
			lw->start -= (lw->rows - 1) / 2;
		else
			lw->start = 0;
		break;

	case CMD_LIST_SCROLL_DOWN_HALF:
		lw->start += (lw->rows - 1) / 2;
		if (lw->start + lw->rows > rows) {
			if (rows > lw->rows)
				lw->start = rows - lw->rows;
			else
				lw->start = 0;
		}
		break;

	default:
		return false;
	}

	return true;
}

#ifdef HAVE_GETMOUSE
bool
list_window_mouse(struct list_window *lw, unsigned rows,
		  unsigned long bstate, int y)
{
	assert(lw != NULL);

	/* if the even occurred above the list window move up */
	if (y < 0) {
		if (bstate & BUTTON3_CLICKED)
			list_window_first(lw);
		else
			list_window_previous_page(lw);
		return true;
	}

	/* if the even occurred below the list window move down */
	if ((unsigned)y >= rows) {
		if (bstate & BUTTON3_CLICKED)
			list_window_last(lw, rows);
		else
			list_window_next_page(lw, rows);
		return true;
	}

	return false;
}
#endif
