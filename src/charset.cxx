// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "charset.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringAPI.hxx"

#include <algorithm>

#include <assert.h>

#ifdef HAVE_ICONV
#include <langinfo.h>
#include <iconv.h>
#include <errno.h>
#endif

#ifdef HAVE_ICONV
static bool noconvert = true;
static const char *charset;

void
charset_init() noexcept
{
	charset = nl_langinfo(CODESET);
	noconvert = charset == nullptr || StringIsEqualIgnoreCase(charset, "utf-8");
}
#endif

static char *
CopyTruncateString(std::span<char> dest, const std::string_view src) noexcept
{
	char *p = std::copy_n(src.begin(), std::min(dest.size() - 1, src.size()), dest.data());
	*p = 0;
	return p;
}

#ifdef HAVE_ICONV

static char *
Iconv(iconv_t i,
      std::span<char> _dest,
      const std::string_view _src) noexcept
{
	static constexpr char FALLBACK = '?';

	char *dest = _dest.data();
	std::size_t dest_size = _dest.size();
	--dest_size; /* reserve once byte for the null terminator */

	char *src = const_cast<char *>(_src.data());
	std::size_t src_length = _src.size();

	while (src_length > 0) {
		size_t err = iconv(i,
				   &src, &src_length,
				   &dest, &dest_size);
		if (err == (size_t)-1) {
			switch (errno) {
			case EILSEQ:
				/* invalid sequence: use fallback
				   character instead */
				++src;
				--src_length;
				*dest++ = FALLBACK;
				break;

			case EINVAL:
				/* incomplete sequence: add fallback
				   character and stop */
				*dest++ = FALLBACK;
				*dest = '\0';
				return dest;

			case E2BIG:
				/* output buffer is full: stop here */
				*dest = '\0';
				return dest;

			default:
				/* unknown error: stop here */
				*dest = '\0';
				return dest;
			}
		}
	}

	*dest = '\0';
	return dest;
}

static char *
Iconv(const char *tocode, const char *fromcode,
      std::span<char> dest,
      const std::string_view src) noexcept
{
	const auto i = iconv_open(tocode, fromcode);
	if (i == (iconv_t)-1) {
		CopyTruncateString(dest, src);
		return dest.data();
	}

	AtScopeExit(i) { iconv_close(i); };

	return Iconv(i, dest, src);
}

[[gnu::pure]]
static std::string
Iconv(iconv_t i, const std::string_view _src) noexcept
{
	static constexpr char FALLBACK = '?';

	std::string dest;

	char *src = const_cast<char *>(_src.data());
	std::size_t src_length = _src.size();

	while (src_length > 0) {
		char buffer[1024], *outbuf = buffer;
		size_t outbytesleft = sizeof(buffer);

		size_t err = iconv(i,
				   &src, &src_length,
				   &outbuf, &outbytesleft);
		dest.append(buffer, outbuf);
		if (err == (size_t)-1) {
			switch (errno) {
			case EILSEQ:
				/* invalid sequence: use fallback
				   character instead */
				++src;
				--src_length;
				dest.push_back(FALLBACK);
				break;

			case EINVAL:
				/* incomplete sequence: add fallback
				   character and stop */
				dest.push_back(FALLBACK);
				return dest;

			case E2BIG:
				/* output buffer is full: flush it */
				break;

			default:
				/* unknown error: stop here */
				return dest;
			}
		}
	}

	return dest;
}

[[gnu::pure]]
static std::string
Iconv(const char *tocode, const char *fromcode,
      const std::string_view src) noexcept
{
	const auto i = iconv_open(tocode, fromcode);
	if (i == (iconv_t)-1)
		return std::string{src};

	AtScopeExit(i) { iconv_close(i); };

	return Iconv(i, src);
}

[[gnu::pure]]
static std::string
utf8_to_locale(const std::string_view src) noexcept
{
	if (noconvert)
		return std::string{src};

	return Iconv(charset, "utf-8", src);
}

#endif

char *
CopyUtf8ToLocale(std::span<char> dest, const std::string_view src) noexcept
{
#ifdef HAVE_ICONV
	if (noconvert) {
#endif
		return CopyTruncateString(dest, src);
#ifdef HAVE_ICONV
	} else {
		return Iconv(charset, "utf-8", dest, src);
	}
#endif
}

const char *
utf8_to_locale(const char *src, std::span<char> buffer) noexcept
{
#ifdef HAVE_ICONV
	CopyUtf8ToLocale(buffer, src);
	return buffer.data();
#else
	(void)buffer;
	return src;
#endif
}

#ifdef HAVE_ICONV

[[gnu::pure]]
static std::string
locale_to_utf8(const std::string_view src) noexcept
{
	if (noconvert)
		return std::string{src};

	return Iconv("utf-8", charset, src);
}

Utf8ToLocale::Utf8ToLocale(const std::string_view src) noexcept
	:value(utf8_to_locale(src)) {}

LocaleToUtf8::LocaleToUtf8(const std::string_view src) noexcept
	:value(locale_to_utf8(src)) {}

#endif
