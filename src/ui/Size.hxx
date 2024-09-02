// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

/**
 * Dimensions of a curses screen/window in cells.
 */
struct Size {
	unsigned width, height;

	constexpr bool operator==(const Size &) const noexcept = default;

	constexpr Size operator+(Size other) const noexcept {
		return {width + other.width, height + other.height};
	}

	constexpr Size operator-(Size other) const noexcept {
		return {width - other.width, height - other.height};
	}
};
