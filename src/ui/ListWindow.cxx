// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ListWindow.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"
#include "config.h"
#include "Match.hxx"
#include "Options.hxx"
#include "Command.hxx"
#include "paint.hxx"
#include "screen_utils.hxx"

void
ListWindow::Paint(const ListRenderer &renderer) const noexcept
{
	bool cursor_visible = IsCursorVisible() &&
		(!options.hardware_cursor || HasRangeSelection());
	ListWindowRange range;

	if (cursor_visible)
		range = GetRange();

	for (unsigned i = 0; i < GetHeight(); i++) {
		window.MoveCursor({0, (int)i});

		const unsigned j = GetOrigin() + i;
		if (j >= GetLength()) {
			window.ClearToBottom();
			break;
		}

		bool is_selected = cursor_visible &&
			range.Contains(j);

		renderer.PaintListItem(window, j, i, width, is_selected);
	}

	row_color_end(window);

	if (options.hardware_cursor && IsVisible(GetCursorIndex())) {
		curs_set(1);
		window.MoveCursor({0, (int)GetCursorIndex() - (int)GetOrigin()});
	}
}

bool
ListWindow::Find(const ListText &text,
		 std::string_view str,
		 bool wrap,
		 bool bell_on_wrap) noexcept
{
	unsigned i = GetCursorIndex() + 1;

	MatchExpression m;
	if (!m.Compile(str, false))
		return false;

	do {
		while (i < GetLength()) {
			char buffer[1024];
			const std::string_view label = text.GetListItemText(buffer, i);

			if (m(label)) {
				MoveCursor(i);
				HighlightCursor();
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
			std::string_view str,
			bool wrap,
			bool bell_on_wrap) noexcept
{
	int i = GetCursorIndex() - 1;

	if (GetLength() == 0)
		return false;

	MatchExpression m;
	if (!m.Compile(str, false))
		return false;

	do {
		while (i >= 0) {
			char buffer[1024];
			const std::string_view label = text.GetListItemText(buffer, i);

			if (m(label)) {
				MoveCursor(i);
				HighlightCursor();
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
ListWindow::Jump(const ListText &text, std::string_view str) noexcept
{
	MatchExpression m;
	if (!m.Compile(str, options.jump_prefix_only))
		return false;

	for (unsigned i = 0; i < GetLength(); i++) {
		char buffer[1024];
		const std::string_view label = text.GetListItemText(buffer, i);

		if (m(label)) {
			MoveCursor(i);
			HighlightCursor();
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
			DisableRangeSelection();
		} else {
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
#if defined(BUTTON4_PRESSED) && defined(BUTTON5_PRESSED)
	if (bstate & BUTTON4_PRESSED) {
		/* mouse wheel up */
		ScrollUp(4);
		return true;
	}

	if (bstate & BUTTON5_PRESSED) {
		/* mouse wheel down */
		ScrollDown(4);
		return true;
	}
#endif

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
