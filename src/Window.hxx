// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Point.hxx"
#include "Size.hxx"

#include <curses.h>

enum class Style : unsigned;

/**
 * A non-owning wrapper for a ncurses #WINDOW pointer.
 */
struct Window {
	WINDOW *w;

	explicit constexpr Window(WINDOW *_w) noexcept:w(_w) {}

	[[gnu::pure]]
	const Size GetSize() const noexcept {
		Size size;
		getmaxyx(w, size.height, size.width);
		return size;
	}

	[[gnu::pure]]
	unsigned GetWidth() const noexcept {
		return getmaxx(w);
	}

	[[gnu::pure]]
	unsigned GetHeight() const noexcept {
		return getmaxy(w);
	}

	void SetBackgroundStyle(Style style) const noexcept {
		wbkgd(w, COLOR_PAIR(unsigned(style)));
	}

	void Move(Point p) const noexcept {
		mvwin(w, p.y, p.x);
	}
};

/**
 * Like #Window, but this class owns the #WINDOW.  The constructor
 * creates a new instance and the destructor deletes it.
 */
struct UniqueWindow : Window {
	UniqueWindow(Point p, Size _size) noexcept
		:Window(newwin(_size.height, _size.width, p.y, p.x)) {}

	~UniqueWindow() noexcept {
		delwin(w);
	}

	UniqueWindow(const UniqueWindow &) = delete;
	UniqueWindow &operator=(const UniqueWindow &) = delete;

	void Resize(Size new_size) noexcept {
		wresize(w, new_size.height, new_size.width);
	}
};
