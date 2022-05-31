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

	void SetBackgroundStyle(Style style) noexcept {
		wbkgd(w, COLOR_PAIR(unsigned(style)));
	}

	void Move(Point p) noexcept {
		mvwin(w, p.y, p.x);
	}

	void Resize(Size new_size) noexcept {
		size = new_size;
		wresize(w, size.height, size.width);
	}
};

#endif
