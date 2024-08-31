// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

#include <string_view>

#include <stddef.h>

#ifdef HAVE_ICONV

#include <string>

void
charset_init() noexcept;
#endif

char *
CopyUtf8ToLocale(char *dest, size_t dest_size, std::string_view src) noexcept;

[[gnu::pure]]
const char *
utf8_to_locale(const char *src, char *buffer, size_t size) noexcept;

/**
 * Convert an UTF-8 string to the locale charset.  The source string
 * must remain valid while this object is used.  If no conversion is
 * necessary, then this class is a no-op.
 */
class Utf8ToLocale {
#ifdef HAVE_ICONV
	const std::string value;
#else
	const char *const value;
#endif

public:
#ifdef HAVE_ICONV
	explicit Utf8ToLocale(const std::string_view src) noexcept;

	Utf8ToLocale(const Utf8ToLocale &) = delete;
	Utf8ToLocale &operator=(const Utf8ToLocale &) = delete;
#else
	explicit Utf8ToLocale(const char *src) noexcept
		:value(src) {}
#endif

	const char *c_str() const noexcept {
#ifdef HAVE_ICONV
		return value.c_str();
#else
		return value;
#endif
	}
};

/**
 * Convert an locale charset string to UTF-8.  The source string must
 * remain valid while this object is used.  If no conversion is
 * necessary, then this class is a no-op.
 */
class LocaleToUtf8 {
#ifdef HAVE_ICONV
	const std::string value;
#else
	const char *const value;
#endif

public:
#ifdef HAVE_ICONV
	explicit LocaleToUtf8(const std::string_view src) noexcept;

	LocaleToUtf8(const LocaleToUtf8 &) = delete;
	LocaleToUtf8 &operator=(const LocaleToUtf8 &) = delete;
#else
	explicit LocaleToUtf8(const char *src) noexcept
		:value(src) {}
#endif

	const char *c_str() const noexcept {
#ifdef HAVE_ICONV
		return value.c_str();
#else
		return value;
#endif
	}
};
