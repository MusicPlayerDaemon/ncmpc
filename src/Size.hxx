// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_SIZE_HXX
#define NCMPC_SIZE_HXX

/**
 * Dimensions of a curses screen/window in cells.
 */
struct Size {
	unsigned width, height;

	constexpr bool operator==(Size other) const noexcept {
		return width == other.width && height == other.height;
	}

	constexpr bool operator!=(Size other) const noexcept {
		return !(*this == other);
	}

	constexpr Size operator+(Size other) const noexcept {
		return {width + other.width, height + other.height};
	}

	constexpr Size operator-(Size other) const noexcept {
		return {width - other.width, height - other.height};
	}
};

#endif
