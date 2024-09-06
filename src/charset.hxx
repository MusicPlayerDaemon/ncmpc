// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

#include <span>
#include <string>
#include <string_view>

#ifdef HAVE_ICONV

void
charset_init() noexcept;
#endif

[[nodiscard]]
char *
CopyUtf8ToLocale(std::span<char> dest, std::string_view src) noexcept;

[[nodiscard]] [[gnu::pure]]
std::string_view
utf8_to_locale(std::string_view src, std::span<char> buffer) noexcept;

/**
 * Convert an UTF-8 string to the locale charset.  The source string
 * must remain valid while this object is used.  If no conversion is
 * necessary, then this class is a no-op.
 */
class Utf8ToLocale {
#ifdef HAVE_ICONV
	std::string value;
#else
	const std::string_view value;
#endif

public:
#ifdef HAVE_ICONV
	[[nodiscard]]
	explicit Utf8ToLocale(const std::string_view src) noexcept;

	Utf8ToLocale(const Utf8ToLocale &) = delete;
	Utf8ToLocale &operator=(const Utf8ToLocale &) = delete;
#else
	[[nodiscard]]
	explicit Utf8ToLocale(std::string_view src) noexcept
		:value(src) {}
#endif

	[[nodiscard]]
	operator std::string_view() const noexcept {
			return value;
	}

#ifdef HAVE_ICONV
	[[nodiscard]] [[gnu::pure]]
	const char *c_str() const noexcept {
		return value.c_str();
	}

	std::string &&str() && noexcept {
		return std::move(value);
	}
#else
	std::string str() && noexcept {
		return std::string{value};
	}
#endif
};

/**
 * Convert an locale charset string to UTF-8.  The source string must
 * remain valid while this object is used.  If no conversion is
 * necessary, then this class is a no-op.
 */
class LocaleToUtf8 {
#ifdef HAVE_ICONV
	std::string value;
#else
	const std::string_view value;
#endif

public:
#ifdef HAVE_ICONV
	[[nodiscard]]
	explicit LocaleToUtf8(const std::string_view src) noexcept;

	LocaleToUtf8(const LocaleToUtf8 &) = delete;
	LocaleToUtf8 &operator=(const LocaleToUtf8 &) = delete;
#else
	[[nodiscard]]
	explicit LocaleToUtf8(std::string_view src) noexcept
		:value(src) {}
#endif

	[[nodiscard]]
	operator std::string_view() const noexcept {
			return value;
	}

#ifdef HAVE_ICONV
	[[nodiscard]] [[gnu::pure]]
	const char *c_str() const noexcept {
		return value.c_str();
	}

	std::string &&str() && noexcept {
		return std::move(value);
	}
#else
	std::string str() && noexcept {
		return std::string{value};
	}
#endif
};

#ifdef HAVE_ICONV

using Utf8ToLocaleZ = Utf8ToLocale;
using LocaleToUtf8Z = LocaleToUtf8;

#else

/**
 * Like #Utf8ToLocaleZ, but return a null-terminated string.
 */
class Utf8ToLocaleZ {
	const char *const value;

public:
	[[nodiscard]]
	explicit constexpr Utf8ToLocaleZ(const char *src) noexcept
		:value(src) {}

	[[nodiscard]]
	explicit Utf8ToLocaleZ(const std::string &src) noexcept
		:Utf8ToLocaleZ(src.c_str()) {}

	[[nodiscard]] [[gnu::pure]]
	constexpr const char *c_str() const noexcept {
		return value;
	}
};

/**
 * Like #LocaleToUtf8Z, but return a null-terminated string.
 */
class LocaleToUtf8Z {
	const char *const value;

public:
	[[nodiscard]]
	explicit constexpr LocaleToUtf8Z(const char *src) noexcept
		:value(src) {}

	[[nodiscard]]
	explicit LocaleToUtf8Z(const std::string &src) noexcept
		:LocaleToUtf8Z(src.c_str()) {}

	[[nodiscard]] [[gnu::pure]]
	constexpr const char *c_str() const noexcept {
		return value;
	}
};

#endif // !HAVE_ICONV
