/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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

#ifndef BASIC_MARQUEE_HXX
#define BASIC_MARQUEE_HXX

#include "util/Compiler.h"

#include <string>

/**
 * This class is used to auto-scroll text which does not fit on the
 * screen.  Call hscroll_init() to initialize the object,
 * hscroll_clear() to free resources, and hscroll_set() to begin
 * scrolling.
 */
class BasicMarquee {
	const char *const separator;

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
	BasicMarquee(const char *_separator) noexcept
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
	bool Set(unsigned width, const char *text) noexcept;

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

	gcc_pure
	std::pair<const char *, size_t> ScrollString() const noexcept;
};

#endif
