/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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
#include "Match.hxx"
#include "Options.hxx"
#include "Command.hxx"
#include "paint.hxx"
#include "screen_status.hxx"
#include "screen_utils.hxx"
#include "i18n.h"

#include <assert.h>

void
ListWindow::Paint(const ListRenderer &renderer) const noexcept
{
	bool show_cursor = HasCursor() &&
		(!options.hardware_cursor || HasRangeSelection());
	ListWindowRange range;

	if (show_cursor)
		range = GetRange();

	for (unsigned i = 0; i < GetHeight(); i++) {
		wmove(w, i, 0);

		const unsigned j = GetOrigin() + i;
		if (j >= GetLength()) {
			wclrtobot(w);
			break;
		}

		bool is_selected = show_cursor &&
			range.Contains(j);

		renderer.PaintListItem(w, j, i, width, is_selected);
	}

	row_color_end(w);

	if (options.hardware_cursor && IsVisible(GetCursorIndex())) {
		curs_set(1);
		wmove(w, GetCursorIndex() - GetOrigin(), 0);
	}
}

bool
ListWindow::Find(const ListText &text,
		 const char *str,
		 bool wrap,
		 bool bell_on_wrap) noexcept
{
	unsigned i = GetCursorIndex() + 1;

	assert(str != nullptr);

	MatchExpression m;
	if (!m.Compile(str, false))
		return false;

	do {
		while (i < GetLength()) {
			char buffer[1024];
			const char *label =
				text.GetListItemText(buffer, sizeof(buffer),
						     i);
			assert(label != nullptr);

			if (m(label)) {
				MoveCursor(i);
				return true;
			}
			if (wrap && i == GetCursorIndex())
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
			bool bell_on_wrap) noexcept
{
	int i = GetCursorIndex() - 1;

	assert(str != nullptr);

	if (GetLength() == 0)
		return false;

	MatchExpression m;
	if (!m.Compile(str, false))
		return false;

	do {
		while (i >= 0) {
			char buffer[1024];
			const char *label =
				text.GetListItemText(buffer, sizeof(buffer),
						     i);
			assert(label != nullptr);

			if (m(label)) {
				MoveCursor(i);
				return true;
			}
			if (wrap && i == (int)GetCursorIndex())
				return false;
			i--;
		}
		if (wrap) {
			i = GetLength() - 1; /* last item */
			if (bell_on_wrap) {
				screen_bell();
			}
		}
	} while (wrap);

	return false;
}

bool
ListWindow::Jump(const ListText &text, const char *str) noexcept
{
	assert(str != nullptr);

	MatchExpression m;
	if (!m.Compile(str, options.jump_prefix_only))
		return false;

	for (unsigned i = 0; i < GetLength(); i++) {
		char buffer[1024];
		const char *label =
			text.GetListItemText(buffer, sizeof(buffer),
					     i);
		assert(label != nullptr);

		if (m(label)) {
			MoveCursor(i);
			return true;
		}
	}

	return false;
}

/* perform basic list window commands (movement) */
bool
ListWindow::HandleCommand(Command cmd) noexcept
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
		if (HasRangeSelection()) {
			screen_status_message(_("Range selection disabled"));
			DisableRangeSelection();
		} else {
			screen_status_message(_("Range selection enabled"));
			EnableRangeSelection();
		}
		break;
	case Command::LIST_SCROLL_UP_LINE:
		ScrollUp(1);
		break;
	case Command::LIST_SCROLL_DOWN_LINE:
		ScrollDown(1);
		break;
	case Command::LIST_SCROLL_UP_HALF:
		ScrollUp((GetHeight() - 1) / 2);
		break;
	case Command::LIST_SCROLL_DOWN_HALF:
		ScrollDown((GetHeight() - 1) / 2);
		break;
	default:
		return false;
	}

	return true;
}

bool
ListWindow::HandleScrollCommand(Command cmd) noexcept
{
	switch (cmd) {
	case Command::LIST_SCROLL_UP_LINE:
	case Command::LIST_PREVIOUS:
		ScrollUp(1);
		break;

	case Command::LIST_SCROLL_DOWN_LINE:
	case Command::LIST_NEXT:
		ScrollDown(1);
		break;

	case Command::LIST_FIRST:
		SetOrigin(0);
		break;

	case Command::LIST_LAST:
		ScrollToBottom();
		break;

	case Command::LIST_NEXT_PAGE:
		ScrollNextPage();
		break;

	case Command::LIST_PREVIOUS_PAGE:
		ScrollPreviousPage();
		break;

	case Command::LIST_SCROLL_UP_HALF:
		ScrollPreviousHalfPage();
		break;

	case Command::LIST_SCROLL_DOWN_HALF:
		ScrollNextHalfPage();
		break;

	default:
		return false;
	}

	return true;
}

#ifdef HAVE_GETMOUSE
bool
ListWindow::HandleMouse(mmask_t bstate, int y) noexcept
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
	if ((unsigned)y >= GetLength()) {
		if (bstate & BUTTON3_CLICKED)
			MoveCursorLast();
		else
			MoveCursorNextPage();
		return true;
	}

	return false;
}
#endif
