// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_POINT_HXX
#define NCMPC_POINT_HXX

#include "Size.hxx"

/**
 * Coordinates of a cell on a curses screen/window.
 */
struct Point {
	int x, y;

	constexpr Point operator+(Point other) const noexcept {
		return {x + other.x, y + other.y};
	}

	constexpr Point operator-(Point other) const noexcept {
		return {x - other.x, y - other.y};
	}

	constexpr Point operator+(Size size) const noexcept {
		return {x + int(size.width), y + int(size.height)};
	}
};

#endif
