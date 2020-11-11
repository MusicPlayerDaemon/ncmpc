/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#include "ListCursor.hxx"
#include "Size.hxx"
#include "config.h"

#include <curses.h>

enum class Command : unsigned;
class ListText;
class ListRenderer;

class ListWindow : public ListCursor {
	WINDOW *const w;

	unsigned width;

public:
	ListWindow(WINDOW *_w, Size _size) noexcept
		:ListCursor(_size.height), w(_w), width(_size.width) {}

	unsigned GetWidth() const noexcept {
		return width;
	}

	void Resize(Size _size) noexcept {
		SetHeight(_size.height);
		width = _size.width;
	}

	void Refresh() const noexcept {
		wrefresh(w);
	}

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
};

#endif
