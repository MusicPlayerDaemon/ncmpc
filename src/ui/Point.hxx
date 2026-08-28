// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Size.hxx"

/**
 * Coordinates of a cell on a curses screen/window.
 */
struct Point {
	int x, y;

	constexpr Point() noexcept = default;
	constexpr Point(int _x, int _y) noexcept:x(_x), y(_y) {}
	constexpr Point(unsigned _x, unsigned _y) noexcept:x(_x), y(_y) {}

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
