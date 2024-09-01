// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string>
#include <string_view>
#include <utility>

/**
 * This class is used to auto-scroll text which does not fit on the
 * screen.  Call Set() to begin scrolling.
 */
class BasicMarquee {
	const std::string_view separator;

	/**
	 * The scrolled text, in the current locale.
	 */
	std::string text;

	/**
	 * A buffer containing the text plus the separator twice.
	 */
	std::string buffer;

	/**
	 * The text plus separator length in characters.
	 */
	size_t max_offset;

	/**
	 * The available screen width (in cells).
	 */
	unsigned width = 0;

	/**
	 * The current scrolling offset.  This is a character
	 * position, not a screen column.
	 */
	unsigned offset;

public:
	explicit BasicMarquee(std::string_view _separator) noexcept
		:separator(_separator) {}

	bool IsDefined() const noexcept {
		return width > 0;
	}

	/**
	 * Sets a text to scroll.  Call Clear() to disable it.
	 *
	 * @param text the text in the locale charset
	 * @return false if nothing was changed
	 */
	bool Set(unsigned width, std::string_view _text) noexcept;

	/**
	 * Removes the text.  It may be reused with Set().
	 */
	void Clear() noexcept;

	void Rewind() noexcept {
		offset = 0;
	}

	void Step() noexcept {
		++offset;

		if (offset >= max_offset)
			offset = 0;
	}

	[[gnu::pure]]
	std::string_view ScrollString() const noexcept;
};
