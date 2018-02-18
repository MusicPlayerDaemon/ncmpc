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

#include "list_window.hxx"
#include "config.h"
#include "options.hxx"
#include "charset.hxx"
#include "match.hxx"
#include "command.hxx"
#include "colors.hxx"
#include "paint.hxx"
#include "screen_status.hxx"
#include "i18n.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern void screen_bell();

ListWindow *
list_window_init(WINDOW *w, unsigned width, unsigned height)
{
	auto *lw = g_new(ListWindow, 1);
	lw->w = w;
	lw->cols = width;
	lw->rows = height;
	lw->range_selection = false;
	return lw;
}

void
list_window_free(ListWindow *lw)
{
	assert(lw != nullptr);

	g_free(lw);
}

void
list_window_reset(ListWindow *lw)
{
	lw->selected = 0;
	lw->range_selection = false;
	lw->range_base = 0;
	lw->start = 0;
}

static unsigned
list_window_validate_index(const ListWindow *lw, unsigned i)
{
	if (lw->length == 0)
		return 0;
	else if (i >= lw->length)
		return lw->length - 1;
	else
		return i;
}

static void
list_window_check_selected(ListWindow *lw)
{
	lw->selected = list_window_validate_index(lw, lw->selected);

	if(lw->range_selection)
		lw->range_base =
			list_window_validate_index(lw, lw->range_base);
}

/**
 * Scroll after the cursor was moved, the list was changed or the
 * window was resized.
 */
static void
list_window_check_origin(ListWindow *lw)
{
	list_window_scroll_to(lw, lw->selected);
}

void
list_window_resize(ListWindow *lw, unsigned width, unsigned height)
{
	lw->cols = width;
	lw->rows = height;

	list_window_check_origin(lw);
}

void
list_window_set_length(ListWindow *lw, unsigned length)
{
	if (length == lw->length)
		return;

	lw->length = length;

	list_window_check_selected(lw);
	list_window_check_origin(lw);
}

void
list_window_center(ListWindow *lw, unsigned n)
{
	if (n > lw->rows / 2)
		lw->start = n - lw->rows / 2;
	else
		lw->start = 0;

	if (lw->start + lw->rows > lw->length) {
		if (lw->rows < lw->length)
			lw->start = lw->length - lw->rows;
		else
			lw->start = 0;
	}
}

void
list_window_scroll_to(ListWindow *lw, unsigned n)
{
	int start = lw->start;

	if ((unsigned) options.scroll_offset * 2 >= lw->rows)
		// Center if the offset is more than half the screen
		start = n - lw->rows / 2;
	else {
		if (n < lw->start + options.scroll_offset)
			start = n - options.scroll_offset;

		if (n >= lw->start + lw->rows - options.scroll_offset)
			start = n - lw->rows + 1 + options.scroll_offset;
	}

	if (start + lw->rows > lw->length)
		start = lw->length - lw->rows;

	if (start < 0 || lw->length == 0)
		start = 0;

	lw->start = start;
}

void
list_window_set_cursor(ListWindow *lw, unsigned i)
{
	lw->range_selection = false;
	lw->selected = i;

	list_window_check_selected(lw);
	list_window_check_origin(lw);
}

void
list_window_move_cursor(ListWindow *lw, unsigned n)
{
	lw->selected = n;

	list_window_check_selected(lw);
	list_window_check_origin(lw);
}

void
list_window_fetch_cursor(ListWindow *lw)
{
	if (lw->start > 0 &&
	    lw->selected < lw->start + options.scroll_offset)
		list_window_move_cursor(lw, lw->start + options.scroll_offset);
	else if (lw->start + lw->rows < lw->length &&
		 lw->selected > lw->start + lw->rows - 1 - options.scroll_offset)
		list_window_move_cursor(lw, lw->start + lw->rows - 1 - options.scroll_offset);
}

void
list_window_get_range(const ListWindow *lw,
		      ListWindowRange *range)
{
	if (lw->length == 0) {
		/* empty list - no selection */
		range->start = 0;
		range->end = 0;
	} else if (lw->range_selection) {
		/* a range selection */
		if (lw->range_base < lw->selected) {
			range->start = lw->range_base;
			range->end = lw->selected + 1;
		} else {
			range->start = lw->selected;
			range->end = lw->range_base + 1;
		}
	} else {
		/* no range, just the cursor */
		range->start = lw->selected;
		range->end = lw->selected + 1;
	}
}

static void
list_window_next(ListWindow *lw)
{
	if (lw->selected + 1 < lw->length)
		list_window_move_cursor(lw, lw->selected + 1);
	else if (options.list_wrap)
		list_window_move_cursor(lw, 0);
}

static void
list_window_previous(ListWindow *lw)
{
	if (lw->selected > 0)
		list_window_move_cursor(lw, lw->selected - 1);
	else if (options.list_wrap)
		list_window_move_cursor(lw, lw->length - 1);
}

static void
list_window_top(ListWindow *lw)
{
	if (lw->start == 0)
		list_window_move_cursor(lw, lw->start);
	else
		if ((unsigned) options.scroll_offset * 2 >= lw->rows)
			list_window_move_cursor(lw, lw->start + lw->rows / 2);
		else
			list_window_move_cursor(lw, lw->start + options.scroll_offset);
}

static void
list_window_middle(ListWindow *lw)
{
	if (lw->length >= lw->rows)
		list_window_move_cursor(lw, lw->start + lw->rows / 2);
	else
		list_window_move_cursor(lw, lw->length / 2);
}

static void
list_window_bottom(ListWindow *lw)
{
	if (lw->length >= lw->rows)
		if ((unsigned) options.scroll_offset * 2 >= lw->rows)
			list_window_move_cursor(lw, lw->start + lw->rows / 2);
		else
			if (lw->start + lw->rows == lw->length)
				list_window_move_cursor(lw, lw->length - 1);
			else
				list_window_move_cursor(lw, lw->start + lw->rows - 1 - options.scroll_offset);
	else
		list_window_move_cursor(lw, lw->length - 1);
}

static void
list_window_first(ListWindow *lw)
{
	list_window_move_cursor(lw, 0);
}

static void
list_window_last(ListWindow *lw)
{
	if (lw->length > 0)
		list_window_move_cursor(lw, lw->length - 1);
	else
		list_window_move_cursor(lw, 0);
}

static void
list_window_next_page(ListWindow *lw)
{
	if (lw->rows < 2)
		return;
	if (lw->selected + lw->rows < lw->length)
		list_window_move_cursor(lw, lw->selected + lw->rows - 1);
	else
		list_window_last(lw);
}

static void
list_window_previous_page(ListWindow *lw)
{
	if (lw->rows < 2)
		return;
	if (lw->selected > lw->rows - 1)
		list_window_move_cursor(lw, lw->selected - lw->rows + 1);
	else
		list_window_first(lw);
}

static void
list_window_scroll_up(ListWindow *lw, unsigned n)
{
	if (lw->start > 0) {
		if (n > lw->start)
			lw->start = 0;
		else
			lw->start -= n;

		list_window_fetch_cursor(lw);
	}
}

static void
list_window_scroll_down(ListWindow *lw, unsigned n)
{
	if (lw->start + lw->rows < lw->length)
	{
		if (lw->start + lw->rows + n > lw->length - 1)
			lw->start = lw->length - lw->rows;
		else
			lw->start += n;

		list_window_fetch_cursor(lw);
	}
}

static void
list_window_paint_row(WINDOW *w, unsigned width, bool selected,
		      const char *text)
{
	row_paint_text(w, width, COLOR_LIST,
		       selected, text);
}

void
list_window_paint(const ListWindow *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data)
{
	bool show_cursor = !lw->hide_cursor &&
		(!options.hardware_cursor || lw->range_selection);
	ListWindowRange range;

	if (show_cursor)
		list_window_get_range(lw, &range);

	for (unsigned i = 0; i < lw->rows; i++) {
		wmove(lw->w, i, 0);

		if (lw->start + i >= lw->length) {
			wclrtobot(lw->w);
			break;
		}

		const char *label = callback(lw->start + i, callback_data);
		assert(label != nullptr);

		list_window_paint_row(lw->w, lw->cols,
				      show_cursor &&
				      lw->start + i >= range.start &&
				      lw->start + i < range.end,
				      label);
	}

	row_color_end(lw->w);

	if (options.hardware_cursor && lw->selected >= lw->start &&
	    lw->selected < lw->start + lw->rows) {
		curs_set(1);
		wmove(lw->w, lw->selected - lw->start, 0);
	}
}

void
list_window_paint2(const ListWindow *lw,
		   list_window_paint_callback_t paint_callback,
		   const void *callback_data)
{
	bool show_cursor = !lw->hide_cursor &&
		(!options.hardware_cursor || lw->range_selection);
	ListWindowRange range;

	if (show_cursor)
		list_window_get_range(lw, &range);

	for (unsigned i = 0; i < lw->rows; i++) {
		wmove(lw->w, i, 0);

		if (lw->start + i >= lw->length) {
			wclrtobot(lw->w);
			break;
		}

		bool selected = show_cursor &&
			lw->start + i >= range.start &&
			lw->start + i < range.end;

		paint_callback(lw->w, lw->start + i, i, lw->cols,
			       selected, callback_data);
	}

	if (options.hardware_cursor && lw->selected >= lw->start &&
	    lw->selected < lw->start + lw->rows) {
		curs_set(1);
		wmove(lw->w, lw->selected - lw->start, 0);
	}
}

bool
list_window_find(ListWindow *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str,
		 bool wrap,
		 bool bell_on_wrap)
{
	unsigned i = lw->selected + 1;

	assert(str != nullptr);

	do {
		while (i < lw->length) {
			const char *label = callback(i, callback_data);
			assert(label != nullptr);

			if (match_line(label, str)) {
				list_window_move_cursor(lw, i);
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
list_window_rfind(ListWindow *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  const char *str,
		  bool wrap,
		  bool bell_on_wrap)
{
	int i = lw->selected - 1;

	assert(str != nullptr);

	if (lw->length == 0)
		return false;

	do {
		while (i >= 0) {
			const char *label = callback(i, callback_data);
			assert(label != nullptr);

			if (match_line(label, str)) {
				list_window_move_cursor(lw, i);
				return true;
			}
			if (wrap && i == (int)lw->selected)
				return false;
			i--;
		}
		if (wrap) {
			i = lw->length - 1; /* last item */
			if (bell_on_wrap) {
				screen_bell();
			}
		}
	} while (wrap);

	return false;
}

#ifdef NCMPC_MINI
bool
list_window_jump(ListWindow *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str)
{
	assert(str != nullptr);

	for (unsigned i = 0; i < lw->length; i++) {
		const char *label = callback(i, callback_data);
		assert(label != nullptr);

		if (g_ascii_strncasecmp(label, str, strlen(str)) == 0) {
			list_window_move_cursor(lw, i);
			return true;
		}
	}
	return false;
}
#else
bool
list_window_jump(ListWindow *lw,
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 const char *str)
{
	assert(str != nullptr);

	GRegex *regex = compile_regex(str, options.jump_prefix_only);
	if (regex == nullptr)
		return false;

	for (unsigned i = 0; i < lw->length; i++) {
		const char *label = callback(i, callback_data);
		assert(label != nullptr);

		if (match_regex(regex, label)) {
			g_regex_unref(regex);
			list_window_move_cursor(lw, i);
			return true;
		}
	}
	g_regex_unref(regex);
	return false;
}
#endif

/* perform basic list window commands (movement) */
bool
list_window_cmd(ListWindow *lw, command_t cmd)
{
	switch (cmd) {
	case CMD_LIST_PREVIOUS:
		list_window_previous(lw);
		break;
	case CMD_LIST_NEXT:
		list_window_next(lw);
		break;
	case CMD_LIST_TOP:
		list_window_top(lw);
		break;
	case CMD_LIST_MIDDLE:
		list_window_middle(lw);
		break;
	case CMD_LIST_BOTTOM:
		list_window_bottom(lw);
		break;
	case CMD_LIST_FIRST:
		list_window_first(lw);
		break;
	case CMD_LIST_LAST:
		list_window_last(lw);
		break;
	case CMD_LIST_NEXT_PAGE:
		list_window_next_page(lw);
		break;
	case CMD_LIST_PREVIOUS_PAGE:
		list_window_previous_page(lw);
		break;
	case CMD_LIST_RANGE_SELECT:
		if(lw->range_selection)
		{
			screen_status_printf(_("Range selection disabled"));
			list_window_set_cursor(lw, lw->selected);
		}
		else
		{
			screen_status_printf(_("Range selection enabled"));
			lw->range_base = lw->selected;
			lw->range_selection = true;
		}
		break;
	case CMD_LIST_SCROLL_UP_LINE:
		list_window_scroll_up(lw, 1);
		break;
	case CMD_LIST_SCROLL_DOWN_LINE:
		list_window_scroll_down(lw, 1);
		break;
	case CMD_LIST_SCROLL_UP_HALF:
		list_window_scroll_up(lw, (lw->rows - 1) / 2);
		break;
	case CMD_LIST_SCROLL_DOWN_HALF:
		list_window_scroll_down(lw, (lw->rows - 1) / 2);
		break;
	default:
		return false;
	}

	return true;
}

bool
list_window_scroll_cmd(ListWindow *lw, command_t cmd)
{
	switch (cmd) {
	case CMD_LIST_SCROLL_UP_LINE:
	case CMD_LIST_PREVIOUS:
		if (lw->start > 0)
			lw->start--;
		break;

	case CMD_LIST_SCROLL_DOWN_LINE:
	case CMD_LIST_NEXT:
		if (lw->start + lw->rows < lw->length)
			lw->start++;
		break;

	case CMD_LIST_FIRST:
		lw->start = 0;
		break;

	case CMD_LIST_LAST:
		if (lw->length > lw->rows)
			lw->start = lw->length - lw->rows;
		else
			lw->start = 0;
		break;

	case CMD_LIST_NEXT_PAGE:
		lw->start += lw->rows;
		if (lw->start + lw->rows > lw->length) {
			if (lw->length > lw->rows)
				lw->start = lw->length - lw->rows;
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
		if (lw->start + lw->rows > lw->length) {
			if (lw->length > lw->rows)
				lw->start = lw->length - lw->rows;
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
list_window_mouse(ListWindow *lw, unsigned long bstate, int y)
{
	assert(lw != nullptr);

	/* if the even occurred above the list window move up */
	if (y < 0) {
		if (bstate & BUTTON3_CLICKED)
			list_window_first(lw);
		else
			list_window_previous_page(lw);
		return true;
	}

	/* if the even occurred below the list window move down */
	if ((unsigned)y >= lw->length) {
		if (bstate & BUTTON3_CLICKED)
			list_window_last(lw);
		else
			list_window_next_page(lw);
		return true;
	}

	return false;
}
#endif
