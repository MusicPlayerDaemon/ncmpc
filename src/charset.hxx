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

#ifndef CHARSET_H
#define CHARSET_H

#include "config.h"
#include "util/Compiler.h"

#include <stddef.h>

#ifdef ENABLE_LOCALE
void
charset_init();
#endif

char *
CopyUtf8ToLocale(char *dest, size_t dest_size, const char *src) noexcept;

char *
CopyUtf8ToLocale(char *dest, size_t dest_size,
		 const char *src, size_t src_length) noexcept;

gcc_pure
const char *
utf8_to_locale(const char *src, char *buffer, size_t size) noexcept;

/**
 * Convert an UTF-8 string to the locale charset.  The source string
 * must remain valid while this object is used.  If no conversion is
 * necessary, then this class is a no-op.
 */
class Utf8ToLocale {
#ifdef ENABLE_LOCALE
	char *const value;
#else
	const char *const value;
#endif

public:
#ifdef ENABLE_LOCALE
	explicit Utf8ToLocale(const char *src) noexcept;
	Utf8ToLocale(const char *src, size_t length) noexcept;

	~Utf8ToLocale();

	Utf8ToLocale(const Utf8ToLocale &) = delete;
	Utf8ToLocale &operator=(const Utf8ToLocale &) = delete;
#else
	explicit Utf8ToLocale(const char *src)
		:value(src) {}
#endif

	const char *c_str() const {
		return value;
	}
};

/**
 * Convert an locale charset string to UTF-8.  The source string must
 * remain valid while this object is used.  If no conversion is
 * necessary, then this class is a no-op.
 */
class LocaleToUtf8 {
#ifdef ENABLE_LOCALE
	char *const value;
#else
	const char *const value;
#endif

public:
#ifdef ENABLE_LOCALE
	explicit LocaleToUtf8(const char *src) noexcept;

	~LocaleToUtf8();

	LocaleToUtf8(const LocaleToUtf8 &) = delete;
	LocaleToUtf8 &operator=(const LocaleToUtf8 &) = delete;
#else
	explicit LocaleToUtf8(const char *src)
		:value(src) {}
#endif

	const char *c_str() const {
		return value;
	}
};

#endif
