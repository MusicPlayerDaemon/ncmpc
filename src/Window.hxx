// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_WINDOW_HXX
#define NCMPC_WINDOW_HXX

#include "Point.hxx"
#include "Size.hxx"

#include <curses.h>

enum class Style : unsigned;

struct Window {
	WINDOW *const w;
	Size size;

	Window(Point p, Size _size) noexcept
		:w(newwin(_size.height, _size.width, p.y, p.x)),
		 size(_size) {}

	~Window() noexcept {
		delwin(w);
	}

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	void SetBackgroundStyle(Style style) const noexcept {
		wbkgd(w, COLOR_PAIR(unsigned(style)));
	}

	void Move(Point p) const noexcept {
		mvwin(w, p.y, p.x);
	}

	void Resize(Size new_size) noexcept {
		size = new_size;
		wresize(w, size.height, size.width);
	}
};

#endif
