// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
