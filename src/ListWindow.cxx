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

#include "ListWindow.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"
#include "config.h"
#include "options.hxx"
#include "charset.hxx"
#include "match.hxx"
#include "Command.hxx"
#include "colors.hxx"
#include "paint.hxx"
#include "screen_status.hxx"
#include "screen_utils.hxx"
#include "i18n.h"

#include <glib.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void
ListWindow::Reset()
{
	selected = 0;
	range_selection = false;
	range_base = 0;
	start = 0;
}

unsigned
ListWindow::ValidateIndex(unsigned i) const
{
	if (length == 0)
		return 0;
	else if (i >= length)
		return length - 1;
	else
		return i;
}

void
ListWindow::CheckSelected()
{
	selected = ValidateIndex(selected);

	if (range_selection)
		range_base = ValidateIndex(range_base);
}

void
ListWindow::Resize(Size _size)
{
	size = _size;
	CheckOrigin();
}

void
ListWindow::SetLength(unsigned _length)
{
	if (_length == length)
		return;

	length = _length;

	CheckSelected();
	CheckOrigin();
}

void
ListWindow::Center(unsigned n)
{
	if (n > size.height / 2)
		start = n - size.height / 2;
	else
		start = 0;

	if (start + size.height > length) {
		if (size.height < length)
			start = length - size.height;
		else
			start = 0;
	}
}

void
ListWindow::ScrollTo(unsigned n)
{
	int new_start = start;

	if ((unsigned) options.scroll_offset * 2 >= size.height)
		// Center if the offset is more than half the screen
		new_start = n - size.height / 2;
	else {
		if (n < start + options.scroll_offset)
			new_start = n - options.scroll_offset;

		if (n >= start + size.height - options.scroll_offset)
			new_start = n - size.height + 1 + options.scroll_offset;
	}

	if (new_start + size.height > length)
		new_start = length - size.height;

	if (new_start < 0 || length == 0)
		new_start = 0;

	start = new_start;
}

void
ListWindow::SetCursor(unsigned i)
{
	range_selection = false;
	selected = i;

	CheckSelected();
	CheckOrigin();
}

void
ListWindow::MoveCursor(unsigned n)
{
	selected = n;

	CheckSelected();
	CheckOrigin();
}

void
ListWindow::FetchCursor()
{
	if (start > 0 &&
	    selected < start + options.scroll_offset)
		MoveCursor(start + options.scroll_offset);
	else if (start + size.height < length &&
		 selected > start + size.height - 1 - options.scroll_offset)
		MoveCursor(start + size.height - 1 - options.scroll_offset);
}

ListWindowRange
ListWindow::GetRange() const
{
	if (length == 0) {
		/* empty list - no selection */
		return {0, 0};
	} else if (range_selection) {
		/* a range selection */
		if (range_base < selected) {
			return {range_base, selected + 1};
		} else {
			return {selected, range_base + 1};
		}
	} else {
		/* no range, just the cursor */
		return {selected, selected + 1};
	}
}

void
ListWindow::MoveCursorNext()
{
	if (selected + 1 < length)
		MoveCursor(selected + 1);
	else if (options.list_wrap)
		MoveCursor(0);
}

void
ListWindow::MoveCursorPrevious()
{
	if (selected > 0)
		MoveCursor(selected - 1);
	else if (options.list_wrap)
		MoveCursor(length - 1);
}

void
ListWindow::MoveCursorTop()
{
	if (start == 0)
		MoveCursor(start);
	else
		if ((unsigned) options.scroll_offset * 2 >= size.height)
			MoveCursor(start + size.height / 2);
		else
			MoveCursor(start + options.scroll_offset);
}

void
ListWindow::MoveCursorMiddle()
{
	if (length >= size.height)
		MoveCursor(start + size.height / 2);
	else
		MoveCursor(length / 2);
}

void
ListWindow::MoveCursorBottom()
{
	if (length >= size.height)
		if ((unsigned) options.scroll_offset * 2 >= size.height)
			MoveCursor(start + size.height / 2);
		else
			if (start + size.height == length)
				MoveCursor(length - 1);
			else
				MoveCursor(start + size.height - 1 - options.scroll_offset);
	else
		MoveCursor(length - 1);
}

void
ListWindow::MoveCursorFirst()
{
	MoveCursor(0);
}

void
ListWindow::MoveCursorLast()
{
	if (length > 0)
		MoveCursor(length - 1);
	else
		MoveCursor(0);
}

void
ListWindow::MoveCursorNextPage()
{
	if (size.height < 2)
		return;
	if (selected + size.height < length)
		MoveCursor(selected + size.height - 1);
	else
		MoveCursorLast();
}

void
ListWindow::MoveCursorPreviousPage()
{
	if (size.height < 2)
		return;
	if (selected > size.height - 1)
		MoveCursor(selected - size.height + 1);
	else
		MoveCursorFirst();
}

void
ListWindow::ScrollUp(unsigned n)
{
	if (start > 0) {
		if (n > start)
			start = 0;
		else
			start -= n;

		FetchCursor();
	}
}

void
ListWindow::ScrollDown(unsigned n)
{
	if (start + size.height < length) {
		if (start + size.height + n > length - 1)
			start = length - size.height;
		else
			start += n;

		FetchCursor();
	}
}

void
ListWindow::Paint(const ListRenderer &renderer) const
{
	bool show_cursor = !hide_cursor &&
		(!options.hardware_cursor || range_selection);
	ListWindowRange range;

	if (show_cursor)
		range = GetRange();

	for (unsigned i = 0; i < size.height; i++) {
		wmove(w, i, 0);

		if (start + i >= length) {
			wclrtobot(w);
			break;
		}

		bool is_selected = show_cursor &&
			range.Contains(start + i);

		renderer.PaintListItem(w, start + i, i, size.width,
				       is_selected);
	}

	row_color_end(w);

	if (options.hardware_cursor && selected >= start &&
	    selected < start + size.height) {
		curs_set(1);
		wmove(w, selected - start, 0);
	}
}

bool
ListWindow::Find(const ListText &text,
		 const char *str,
		 bool wrap,
		 bool bell_on_wrap)
{
	unsigned i = selected + 1;

	assert(str != nullptr);

	do {
		while (i < length) {
			char buffer[1024];
			const char *label =
				text.GetListItemText(buffer, sizeof(buffer),
						     i);
			assert(label != nullptr);

			if (match_line(label, str)) {
				MoveCursor(i);
				return true;
			}
			if (wrap && i == selected)
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
ListWindow::ReverseFind(const ListText &text,
			const char *str,
			bool wrap,
			bool bell_on_wrap)
{
	int i = selected - 1;

	assert(str != nullptr);

	if (length == 0)
		return false;

	do {
		while (i >= 0) {
			char buffer[1024];
			const char *label =
				text.GetListItemText(buffer, sizeof(buffer),
						     i);
			assert(label != nullptr);

			if (match_line(label, str)) {
				MoveCursor(i);
				return true;
			}
			if (wrap && i == (int)selected)
				return false;
			i--;
		}
		if (wrap) {
			i = length - 1; /* last item */
			if (bell_on_wrap) {
				screen_bell();
			}
		}
	} while (wrap);

	return false;
}

#ifdef NCMPC_MINI
bool
ListWindow::Jump(const ListText &text, const char *str)
{
	assert(str != nullptr);

	for (unsigned i = 0; i < length; i++) {
		char buffer[1024];
		const char *label =
			text.GetListItemText(buffer, sizeof(buffer),
					     i);
		assert(label != nullptr);

		if (g_ascii_strncasecmp(label, str, strlen(str)) == 0) {
			MoveCursor(i);
			return true;
		}
	}
	return false;
}
#else
bool
ListWindow::Jump(const ListText &text, const char *str)
{
	assert(str != nullptr);

	GRegex *regex = compile_regex(str, options.jump_prefix_only);
	if (regex == nullptr)
		return false;

	for (unsigned i = 0; i < length; i++) {
		char buffer[1024];
		const char *label =
			text.GetListItemText(buffer, sizeof(buffer),
					     i);
		assert(label != nullptr);

		if (match_regex(regex, label)) {
			g_regex_unref(regex);
			MoveCursor(i);
			return true;
		}
	}
	g_regex_unref(regex);
	return false;
}
#endif

/* perform basic list window commands (movement) */
bool
ListWindow::HandleCommand(Command cmd)
{
	switch (cmd) {
	case Command::LIST_PREVIOUS:
		MoveCursorPrevious();
		break;
	case Command::LIST_NEXT:
		MoveCursorNext();
		break;
	case Command::LIST_TOP:
		MoveCursorTop();
		break;
	case Command::LIST_MIDDLE:
		MoveCursorMiddle();
		break;
	case Command::LIST_BOTTOM:
		MoveCursorBottom();
		break;
	case Command::LIST_FIRST:
		MoveCursorFirst();
		break;
	case Command::LIST_LAST:
		MoveCursorLast();
		break;
	case Command::LIST_NEXT_PAGE:
		MoveCursorNextPage();
		break;
	case Command::LIST_PREVIOUS_PAGE:
		MoveCursorPreviousPage();
		break;
	case Command::LIST_RANGE_SELECT:
		if(range_selection)
		{
			screen_status_message(_("Range selection disabled"));
			SetCursor(selected);
		}
		else
		{
			screen_status_message(_("Range selection enabled"));
			range_base = selected;
			range_selection = true;
		}
		break;
	case Command::LIST_SCROLL_UP_LINE:
		ScrollUp(1);
		break;
	case Command::LIST_SCROLL_DOWN_LINE:
		ScrollDown(1);
		break;
	case Command::LIST_SCROLL_UP_HALF:
		ScrollUp((size.height - 1) / 2);
		break;
	case Command::LIST_SCROLL_DOWN_HALF:
		ScrollDown((size.height - 1) / 2);
		break;
	default:
		return false;
	}

	return true;
}

bool
ListWindow::HandleScrollCommand(Command cmd)
{
	switch (cmd) {
	case Command::LIST_SCROLL_UP_LINE:
	case Command::LIST_PREVIOUS:
		if (start > 0)
			start--;
		break;

	case Command::LIST_SCROLL_DOWN_LINE:
	case Command::LIST_NEXT:
		if (start + size.height < length)
			start++;
		break;

	case Command::LIST_FIRST:
		start = 0;
		break;

	case Command::LIST_LAST:
		if (length > size.height)
			start = length - size.height;
		else
			start = 0;
		break;

	case Command::LIST_NEXT_PAGE:
		start += size.height;
		if (start + size.height > length) {
			if (length > size.height)
				start = length - size.height;
			else
				start = 0;
		}
		break;

	case Command::LIST_PREVIOUS_PAGE:
		if (start > size.height)
			start -= size.height;
		else
			start = 0;
		break;

	case Command::LIST_SCROLL_UP_HALF:
		if (start > (size.height - 1) / 2)
			start -= (size.height - 1) / 2;
		else
			start = 0;
		break;

	case Command::LIST_SCROLL_DOWN_HALF:
		start += (size.height - 1) / 2;
		if (start + size.height > length) {
			if (length > size.height)
				start = length - size.height;
			else
				start = 0;
		}
		break;

	default:
		return false;
	}

	return true;
}

#ifdef HAVE_GETMOUSE
bool
ListWindow::HandleMouse(mmask_t bstate, int y)
{
	/* if the even occurred above the list window move up */
	if (y < 0) {
		if (bstate & BUTTON3_CLICKED)
			MoveCursorFirst();
		else
			MoveCursorPreviousPage();
		return true;
	}

	/* if the even occurred below the list window move down */
	if ((unsigned)y >= length) {
		if (bstate & BUTTON3_CLICKED)
			MoveCursorLast();
		else
			MoveCursorNextPage();
		return true;
	}

	return false;
}
#endif
