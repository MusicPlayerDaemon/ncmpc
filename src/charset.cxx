// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "charset.hxx"
#include "util/StringAPI.hxx"

#include <algorithm>

#include <assert.h>

#ifdef ENABLE_CHARSET
#include "util/LocaleConvert.hxx"
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

	return ConvertUtf8ToLocaleString(src);
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
	return ConvertUtf8ToLocaleTruncate(dest, src);
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

	return ConvertLocaleToUtf8String(src);
}

Utf8ToLocale::Utf8ToLocale(const std::string_view src) noexcept
	:value(utf8_to_locale(src)) {}

LocaleToUtf8::LocaleToUtf8(const std::string_view src) noexcept
	:value(locale_to_utf8(src)) {}

#endif // ENABLE_CHARSET
