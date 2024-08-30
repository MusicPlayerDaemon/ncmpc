// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Point.hxx"
#include "Size.hxx"

#include <curses.h>

enum class Style : unsigned;

struct Window {
	WINDOW *const w;

	Window(Point p, Size _size) noexcept
		:w(newwin(_size.height, _size.width, p.y, p.x)) {}

	~Window() noexcept {
		delwin(w);
	}

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

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

	void Resize(Size size) noexcept {
		wresize(w, size.height, size.width);
	}
};
