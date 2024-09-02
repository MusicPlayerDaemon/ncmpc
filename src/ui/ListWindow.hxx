// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "ListCursor.hxx"
#include "Size.hxx"
#include "Window.hxx"
#include "config.h"

enum class Command : unsigned;
class ListText;
class ListRenderer;

class ListWindow : public ListCursor {
	const Window window;

	unsigned width;

public:
	ListWindow(Window _window, Size _size) noexcept
		:ListCursor(_size.height), window(_window), width(_size.width) {}

	unsigned GetWidth() const noexcept {
		return width;
	}

	void Resize(Size _size) noexcept {
		SetHeight(_size.height);
		width = _size.width;
	}

	void Refresh() const noexcept {
		window.Refresh();
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
		  std::string_view str) noexcept;

	/**
	 * Find a string in a list window (reversed).
	 */
	bool ReverseFind(const ListText &text,
			 std::string_view str) noexcept;

	/**
	 * Find a string in a list window which begins with the given
	 * characters in *str.
	 */
	bool Jump(const ListText &text, std::string_view str) noexcept;
};
