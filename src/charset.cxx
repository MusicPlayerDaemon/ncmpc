// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "charset.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringAPI.hxx"

#include <algorithm>

#include <assert.h>

#ifdef ENABLE_CHARSET
#include <cuchar>

/**
 * Replacement definition for MB_CUR_MAX which is not a constant
 * expression; 8 is generous, because 4 is the practical limit
 */
static constexpr std::size_t MAX_MB_SIZE = 8;

#endif

#ifdef ENABLE_LOCALE
#include <langinfo.h>

static bool noconvert = true;

[[gnu::pure]]
static bool
IsUTF8(const char *charset) noexcept
{
	assert(charset != nullptr);

	return StringIsEqualIgnoreCase(charset, "utf-8") ||
		StringIsEqualIgnoreCase(charset, "utf8");
}

void
charset_init() noexcept
{
	const char *charset = nl_langinfo(CODESET);
	noconvert = charset == nullptr || IsUTF8(charset);
}
#endif

[[nodiscard]]
static char *
CopyTruncateString(std::span<char> dest, const std::string_view src) noexcept
{
	assert(!dest.empty());

	return std::copy_n(src.begin(), std::min(dest.size(), src.size()), dest.data());
}

#ifdef ENABLE_CHARSET

[[gnu::pure]]
static std::string
utf8_to_locale(const std::string_view src) noexcept
{
	if (noconvert)
		return std::string{src};

	std::string result;
	result.reserve(src.size());

	mbstate_t state{};
	for (unsigned char byte : src) {
		char buffer[MAX_MB_SIZE];

		std::size_t n = std::c8rtomb(buffer, static_cast<char8_t>(byte), &state);
		if (n == static_cast<std::size_t>(-1)) {
			result.push_back('?');
			state = {};
		} else if (n > 0) {
			result.append(buffer, n);
		}
	}

	return result;
}

#endif

char *
CopyUtf8ToLocale(std::span<char> dest, const std::string_view src) noexcept
{
#ifdef ENABLE_CHARSET
	if (noconvert)
#endif
		return CopyTruncateString(dest, src);

#ifdef ENABLE_CHARSET
	assert(dest.size() > MAX_MB_SIZE);

	mbstate_t state{};
	char *p = dest.data();
	const char *const end = p + dest.size() - MAX_MB_SIZE;

	for (unsigned char byte : src) {
		if (p > end)
			break;

		std::size_t result = std::c8rtomb(p, static_cast<char8_t>(byte), &state);
		if (result == static_cast<std::size_t>(-1)) {
			*p++ = '?';
			state = {};
		} else if (result > 0) {
			p += result;
		}
	}

	return p;
#endif // ENABLE_CHARSET
}

std::string_view
utf8_to_locale(std::string_view src, [[maybe_unused]] std::span<char> buffer) noexcept
{
#ifdef ENABLE_CHARSET
	if (noconvert)
#endif
		return src;

#ifdef ENABLE_CHARSET
	char *end = CopyUtf8ToLocale(buffer, src);
	return {buffer.data(), end};
#endif
}

#ifdef ENABLE_CHARSET

[[gnu::pure]]
static std::string
locale_to_utf8(std::string_view src) noexcept
{
	if (noconvert)
		return std::string{src};

	std::string result;
	result.reserve(src.size());

	mbstate_t state{};

	while (!src.empty()) {
		char8_t c8;
		std::size_t n = std::mbrtoc8(&c8, src.data(), src.size(), &state);
		if (n == static_cast<std::size_t>(-1)) {
			// error
			result.push_back('?');
			src = src.substr(1);
			state = {};
		} else if (n == static_cast<std::size_t>(-2)) {
			// incomplete sequence at end
			result.push_back('?');
			break;
		} else if (n == static_cast<std::size_t>(-3)) {
			// no input consumed yet
			result.push_back(static_cast<char>(c8));
		} else if (n == 0) {
			// null character
			result.push_back('\0');
			src = src.substr(1);
		} else {
			// n bytes consumed from input, output UTF-8 code unit
			result.push_back(static_cast<char>(c8));
			src = src.substr(n);
		}
	}

	return result;
}

Utf8ToLocale::Utf8ToLocale(const std::string_view src) noexcept
	:value(utf8_to_locale(src)) {}

LocaleToUtf8::LocaleToUtf8(const std::string_view src) noexcept
	:value(locale_to_utf8(src)) {}

#endif // ENABLE_CHARSET
