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
CopyTruncateString(char *dest, size_t dest_size,
		   const char *src, size_t src_length) noexcept
{
	dest = std::copy_n(src, std::min(dest_size - 1, src_length), dest);
	*dest = 0;
	return dest;
}

#ifdef HAVE_ICONV

static char *
Iconv(iconv_t i,
      char *dest, size_t dest_size,
      const char *src, size_t src_length) noexcept
{
	static constexpr char FALLBACK = '?';

	--dest_size; /* reserve once byte for the null terminator */

	while (src_length > 0) {
		size_t err = iconv(i,
				   const_cast<char **>(&src), &src_length,
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
      char *dest, size_t dest_size,
      const char *src, size_t src_length) noexcept
{
	const auto i = iconv_open(tocode, fromcode);
	if (i == (iconv_t)-1) {
		CopyTruncateString(dest, dest_size, src, src_length);
		return dest;
	}

	AtScopeExit(i) { iconv_close(i); };

	return Iconv(i, dest, dest_size, src, src_length);
}

[[gnu::pure]]
static std::string
Iconv(iconv_t i,
      const char *src, size_t src_length) noexcept
{
	static constexpr char FALLBACK = '?';

	std::string dest;

	while (src_length > 0) {
		char buffer[1024], *outbuf = buffer;
		size_t outbytesleft = sizeof(buffer);

		size_t err = iconv(i,
				   const_cast<char **>(&src), &src_length,
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
      const char *src, size_t src_length) noexcept
{
	const auto i = iconv_open(tocode, fromcode);
	if (i == (iconv_t)-1)
		return {src, src_length};

	AtScopeExit(i) { iconv_close(i); };

	return Iconv(i, src, src_length);
}

[[gnu::pure]]
static std::string
utf8_to_locale(const char *src, size_t length) noexcept
{
	assert(src != nullptr);

	if (noconvert)
		return {src, length};

	return Iconv(charset, "utf-8",
		     src, length);
}

#endif

char *
CopyUtf8ToLocale(char *dest, size_t dest_size, const char *src) noexcept
{
	return CopyUtf8ToLocale(dest, dest_size, src, strlen(src));
}

char *
CopyUtf8ToLocale(char *dest, size_t dest_size,
		 const char *src, size_t src_length) noexcept
{
#ifdef HAVE_ICONV
	if (noconvert) {
#endif
		return CopyTruncateString(dest, dest_size, src, src_length);
#ifdef HAVE_ICONV
	} else {
		return Iconv(charset, "utf-8", dest, dest_size,
			     src, src_length);
	}
#endif
}

const char *
utf8_to_locale(const char *src, char *buffer, size_t size) noexcept
{
#ifdef HAVE_ICONV
	CopyUtf8ToLocale(buffer, size, src);
	return buffer;
#else
	(void)buffer;
	(void)size;
	return src;
#endif
}

#ifdef HAVE_ICONV

[[gnu::pure]]
static std::string
locale_to_utf8(const char *src) noexcept
{
	assert(src != nullptr);

	if (noconvert)
		return src;

	return Iconv("utf-8", charset,
		     src, strlen(src));
}

Utf8ToLocale::Utf8ToLocale(const char *src) noexcept
	:Utf8ToLocale(src, strlen(src)) {}

Utf8ToLocale::Utf8ToLocale(const char *src, size_t length) noexcept
	:value(utf8_to_locale(src, length)) {}

LocaleToUtf8::LocaleToUtf8(const char *src) noexcept
	:value(locale_to_utf8(src)) {}

#endif
