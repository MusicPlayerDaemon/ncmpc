/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "ListCursor.hxx"
#include "Options.hxx"

ListCursor::ListCursor(unsigned _height) noexcept
	:height(_height),
	 scroll_offset(ClampScrollOffset(options.scroll_offset, height))
{
}

void
ListCursor::Reset() noexcept
{
	selected = 0;
	range_selection = false;
	highlight_cursor = false;
	range_base = 0;
	start = 0;
}

unsigned
ListCursor::ValidateIndex(unsigned i) const noexcept
{
	if (length == 0)
		return 0;
	else if (i >= length)
		return length - 1;
	else
		return i;
}

void
ListCursor::CheckSelected() noexcept
{
	selected = ValidateIndex(selected);

	if (range_selection)
		range_base = ValidateIndex(range_base);
}

void
ListCursor::SetHeight(unsigned _height) noexcept
{
	height = _height;
	scroll_offset = ClampScrollOffset(options.scroll_offset, height);

	CheckOrigin();
}

void
ListCursor::SetLength(unsigned _length) noexcept
{
	if (_length == length)
		return;

	length = _length;

	CheckSelected();
	CheckOrigin();
}

void
ListCursor::Center(unsigned n) noexcept
{
	highlight_cursor = false;

	if (n > GetHeight() / 2)
		start = n - GetHeight() / 2;
	else
		start = 0;

	if (start + GetHeight() > length) {
		if (GetHeight() < length)
			start = length - GetHeight();
		else
			start = 0;
	}
}

void
ListCursor::ScrollTo(unsigned n) noexcept
{
	highlight_cursor = false;

	int new_start = start;

	if (n < start + scroll_offset)
		new_start = n - scroll_offset;

	if (n >= start + GetHeight() - scroll_offset)
		new_start = n - GetHeight() + 1 + scroll_offset;

	if (scroll_offset * 2 == GetHeight())
		new_start = n - GetHeight() + scroll_offset;

	if (new_start + GetHeight() > length)
		new_start = length - GetHeight();

	if (new_start < 0 || length == 0)
		new_start = 0;

	start = new_start;
}

void
ListCursor::SetCursor(unsigned i) noexcept
{
	range_selection = false;
	selected = i;

	CheckSelected();
	CheckOrigin();
}

void
ListCursor::MoveCursor(unsigned n) noexcept
{
	selected = n;
	highlight_cursor = false;

	CheckSelected();
	CheckOrigin();
}

void
ListCursor::FetchCursor() noexcept
{
	if (start > 0 &&
	    selected < start + scroll_offset)
		MoveCursor(start + scroll_offset);
	else if (start + GetHeight() < length &&
		 selected > start + GetHeight() - 1 - scroll_offset)
		MoveCursor(start + GetHeight() - 1 - scroll_offset);
}

ListWindowRange
ListCursor::GetRange() const noexcept
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
ListCursor::MoveCursorNext() noexcept
{
	if (selected + 1 < length)
		MoveCursor(selected + 1);
	else if (options.list_wrap)
		MoveCursor(0);
}

void
ListCursor::MoveCursorPrevious() noexcept
{
	if (selected > 0)
		MoveCursor(selected - 1);
	else if (options.list_wrap)
		MoveCursor(length - 1);
}

void
ListCursor::MoveCursorTop() noexcept
{
	if (start == 0)
		MoveCursor(start);
	else
		MoveCursor(start + scroll_offset);
}

void
ListCursor::MoveCursorMiddle() noexcept
{
	if (length >= GetHeight())
		MoveCursor(start + GetHeight() / 2);
	else
		MoveCursor(length / 2);
}

void
ListCursor::MoveCursorBottom() noexcept
{
	if (length >= GetHeight())
		if (start + GetHeight() == length)
			MoveCursor(length - 1);
		else if (scroll_offset * 2 == GetHeight())
			MoveCursor(start + GetHeight() - scroll_offset);
		else
			MoveCursor(start + GetHeight() - 1 - scroll_offset);
	else
		MoveCursor(length - 1);
}

void
ListCursor::MoveCursorFirst() noexcept
{
	MoveCursor(0);
}

void
ListCursor::MoveCursorLast() noexcept
{
	if (length > 0)
		MoveCursor(length - 1);
	else
		MoveCursor(0);
}

void
ListCursor::MoveCursorNextPage() noexcept
{
	highlight_cursor = false;

	if (GetHeight() < 2)
		return;
	if (selected + GetHeight() < length)
		MoveCursor(selected + GetHeight() - 1);
	else
		MoveCursorLast();
}

void
ListCursor::MoveCursorPreviousPage() noexcept
{
	highlight_cursor = false;

	if (GetHeight() < 2)
		return;
	if (selected > GetHeight() - 1)
		MoveCursor(selected - GetHeight() + 1);
	else
		MoveCursorFirst();
}

void
ListCursor::ScrollUp(unsigned n) noexcept
{
	highlight_cursor = false;

	if (start > 0) {
		if (n > start)
			start = 0;
		else
			start -= n;

		FetchCursor();
	}
}

void
ListCursor::ScrollDown(unsigned n) noexcept
{
	highlight_cursor = false;

	if (start + GetHeight() < length) {
		if (start + GetHeight() + n > length - 1)
			start = length - GetHeight();
		else
			start += n;

		FetchCursor();
	}
}

void
ListCursor::ScrollNextPage() noexcept
{
	highlight_cursor = false;

	start += GetHeight();
	if (start + GetHeight() > length)
		start = length > GetHeight()
			? GetLength() - GetHeight()
			: 0;
}

void
ListCursor::ScrollPreviousPage() noexcept
{
	highlight_cursor = false;

	start = start > GetHeight()
		? start - GetHeight()
		: 0;
}

void
ListCursor::ScrollNextHalfPage() noexcept
{
	highlight_cursor = false;

	start += (GetHeight() - 1) / 2;
	if (start + GetHeight() > length) {
		start = length > GetHeight()
			? length - GetHeight()
			: 0;
	}
}

void
ListCursor::ScrollPreviousHalfPage() noexcept
{
	highlight_cursor = false;

	start = start > (GetHeight() - 1) / 2
		? start - (GetHeight() - 1) / 2
		: 0;
}
