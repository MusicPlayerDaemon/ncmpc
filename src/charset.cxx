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

#include "charset.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringUTF8.hxx"

#include <algorithm>

#include <assert.h>
#include <string.h>
#include <glib.h>

#ifdef ENABLE_LOCALE
static bool noconvert = true;
static const char *charset;

const char *
charset_init()
{
	noconvert = g_get_charset(&charset);
	return charset;
}
#endif

static char *
CopyTruncateString(char *dest, size_t dest_size, const char *src) noexcept
{
	dest = std::copy_n(src, std::min(dest_size - 1, strlen(src)), dest);
	*dest = 0;
	return dest;
}

char *
utf8_to_locale(const char *utf8str)
{
#ifdef ENABLE_LOCALE
	assert(utf8str != nullptr);

	if (noconvert)
		return g_strdup(utf8str);

	gchar *str = g_convert_with_fallback(utf8str, -1,
					     charset, "utf-8",
					     nullptr, nullptr, nullptr, nullptr);
	if (str == nullptr)
		return g_strdup(utf8str);

	return str;
#else
	return g_strdup(utf8str);
#endif
}

char *
CopyUtf8ToLocale(char *dest, size_t dest_size, const char *src) noexcept
{
	return CopyTruncateString(dest, dest_size, Utf8ToLocale(src).c_str());
}

char *
CopyUtf8ToLocale(char *dest, size_t dest_size,
		 const char *src, size_t src_length) noexcept
{
	char *truncated_src = g_strndup(src, src_length);
	dest = CopyUtf8ToLocale(dest, dest_size, truncated_src);
	g_free(truncated_src);
	return dest;
}

const char *
utf8_to_locale(const char *src, char *buffer, size_t size) noexcept
{
#ifdef ENABLE_LOCALE
	CopyUtf8ToLocale(buffer, size, src);
	return buffer;
#else
	(void)buffer;
	(void)size;
	return src;
#endif
}

char *
locale_to_utf8(const char *localestr)
{
#ifdef ENABLE_LOCALE
	assert(localestr != nullptr);

	if (noconvert)
		return g_strdup(localestr);

	gchar *str = g_convert_with_fallback(localestr, -1,
					     "utf-8", charset,
					     nullptr, nullptr, nullptr, nullptr);
	if (str == nullptr)
		return g_strdup(localestr);

	return str;
#else
	return g_strdup(localestr);
#endif
}

char *
replace_utf8_to_locale(char *src)
{
#ifdef ENABLE_LOCALE
	assert(src != nullptr);

	if (noconvert)
		return src;

	AtScopeExit(src) { g_free(src); };
	return utf8_to_locale(src);
#else
	return src;
#endif
}


char *
replace_locale_to_utf8(char *src)
{
#ifdef ENABLE_LOCALE
	assert(src != nullptr);

	if (noconvert)
		return src;

	AtScopeExit(src) { g_free(src); };
	return locale_to_utf8(src);
#else
	return src;
#endif
}

#ifdef ENABLE_LOCALE

Utf8ToLocale::~Utf8ToLocale()
{
	g_free(value);
}

LocaleToUtf8::~LocaleToUtf8()
{
	g_free(value);
}

#endif
