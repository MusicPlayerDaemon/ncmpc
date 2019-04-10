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

#ifndef LIST_WINDOW_HXX
#define LIST_WINDOW_HXX

#include "config.h"
#include "Size.hxx"
#include "util/Compiler.h"

#include <curses.h>

enum class Command : unsigned;
class ListText;
class ListRenderer;

/**
 * The bounds of a range selection, see list_window_get_range().
 */
struct ListWindowRange {
	/**
	 * The index of the first selected item.
	 */
	unsigned start_index;

	/**
	 * The index after the last selected item.  The selection is
	 * empty when this is the same as "start".
	 */
	unsigned end_index;

	constexpr bool empty() const noexcept {
		return start_index >= end_index;
	}

	constexpr bool Contains(unsigned i) const noexcept {
		return i >= start_index && i < end_index;
	}

	struct const_iterator {
		unsigned value;

		const_iterator &operator++() noexcept {
			++value;
			return *this;
		}

		constexpr bool operator==(const const_iterator &other) const noexcept {
			return value == other.value;
		}

		constexpr bool operator!=(const const_iterator &other) const noexcept {
			return !(*this == other);
		}

		const unsigned &operator *() const noexcept {
			return value;
		}
	};

	constexpr const_iterator begin() const noexcept {
		return {start_index};
	}

	constexpr const_iterator end() const noexcept {
		return {end_index};
	}
};

class ListWindow {
	WINDOW *w;
	Size size;

	/**
	 * Number of items in this list.
	 */
	unsigned length = 0;

	unsigned start = 0;
	unsigned selected = 0;

	/**
	 * Represents the base item.
	 */
	unsigned range_base = 0;

	/**
	 * Range selection activated?
	 */
	bool range_selection = false;

	bool hide_cursor = false;

public:
	ListWindow(WINDOW *_w, Size _size) noexcept
		:w(_w), size(_size) {}

	const Size &GetSize() const noexcept {
		return size;
	}

	void Refresh() const noexcept {
		wrefresh(w);
	}

	unsigned GetOrigin() const noexcept {
		return start;
	}

	void SetOrigin(unsigned new_orign) noexcept {
		start = new_orign;
	}

	void DisableCursor() {
		hide_cursor = true;
	}

	void EnableCursor() {
		hide_cursor = false;
	}

	bool HasCursor() const noexcept {
		return !hide_cursor;
	}

	bool HasRangeSelection() const noexcept {
		return range_selection;
	}

	/**
	 * Is the cursor currently pointing to a single valid item?
	 */
	bool IsSingleCursor() const noexcept {
		return !HasRangeSelection() && selected < length;
	}

	unsigned GetCursorIndex() const noexcept {
		return selected;
	}

	void SelectionMovedUp() noexcept {
		selected--;
		range_base--;

		EnsureSelectionVisible();
	}

	void SelectionMovedDown() noexcept {
		selected++;
		range_base++;

		EnsureSelectionVisible();
	}

	void EnsureSelectionVisible() noexcept {
		if (range_selection)
			ScrollTo(range_base);
		ScrollTo(selected);
	}

	/** reset a list window (selected=0, start=0) */
	void Reset() noexcept;

	void Resize(Size _size) noexcept;

	void SetLength(unsigned length) noexcept;

	void Paint(const ListRenderer &renderer) const noexcept;

	/** perform basic list window commands (movement) */
	bool HandleCommand(Command cmd) noexcept;

	/**
	 * Scroll the window.  Returns true if the command has been
	 * consumed.
	 */
	bool HandleScrollCommand(Command cmd) noexcept;

#ifdef HAVE_GETMOUSE
	/**
	 * The mouse was clicked.  Check if the list should be scrolled
	 * Returns non-zero if the mouse event has been handled.
	 */
	bool HandleMouse(mmask_t bstate, int y) noexcept;
#endif

	/**
	 * Centers the visible range around item n on the list.
	 */
	void Center(unsigned n) noexcept;

	/**
	 * Scrolls the view to item n, as if the cursor would have been moved
	 * to the position.
	 */
	void ScrollTo(unsigned n) noexcept;

	/**
	 * Sets the position of the cursor.  Disables range selection.
	 */
	void SetCursor(unsigned i) noexcept;

	void SetCursorFromOrigin(unsigned i) noexcept {
		SetCursor(GetOrigin() + i);
	}

	/**
	 * Moves the cursor.  Modifies the range if range selection is
	 * enabled.
	 */
	void MoveCursor(unsigned n) noexcept;

	void MoveCursorNext() noexcept;
	void MoveCursorPrevious() noexcept;
	void MoveCursorTop() noexcept;
	void MoveCursorMiddle() noexcept;
	void MoveCursorBottom() noexcept;
	void MoveCursorFirst() noexcept;
	void MoveCursorLast() noexcept;
	void MoveCursorNextPage() noexcept;
	void MoveCursorPreviousPage() noexcept;

	void ScrollUp(unsigned n) noexcept;
	void ScrollDown(unsigned n) noexcept;

	/**
	 * Ensures that the cursor is visible on the screen, i.e. it is not
	 * outside the current scrolling range.
	 */
	void FetchCursor() noexcept;

	/**
	 * Determines the lower and upper bound of the range selection.  If
	 * range selection is disabled, it returns the cursor position (range
	 * length is 1).
	 */
	gcc_pure
	ListWindowRange GetRange() const noexcept;

	/**
	 * Find a string in a list window.
	 */
	bool Find(const ListText &text,
		  const char *str,
		  bool wrap,
		  bool bell_on_wrap) noexcept;

	/**
	 * Find a string in a list window (reversed).
	 */
	bool ReverseFind(const ListText &text,
			 const char *str,
			 bool wrap,
			 bool bell_on_wrap) noexcept;

	/**
	 * Find a string in a list window which begins with the given
	 * characters in *str.
	 */
	bool Jump(const ListText &text, const char *str) noexcept;

private:
	gcc_pure
	unsigned ValidateIndex(unsigned i) const noexcept;

	void CheckSelected() noexcept;

	/**
	 * Scroll after the cursor was moved, the list was changed or
	 * the window was resized.
	 */
	void CheckOrigin() noexcept {
		ScrollTo(selected);
	}
};

#endif
