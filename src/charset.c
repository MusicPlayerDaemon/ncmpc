/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "charset.h"

#include <assert.h>
#include <string.h>
#include <glib.h>

#ifdef ENABLE_LOCALE
static bool noconvert = true;
static const char *charset;

const char *
charset_init(void)
{
	noconvert = g_get_charset(&charset);
	return charset;
}
#endif

#ifdef ENABLE_WIDE
static inline unsigned
unicode_char_width(gunichar ch)
{
#if GLIB_CHECK_VERSION(2,14,0)
	if (g_unichar_iszerowidth(ch))
		return 0;
#endif

	if (g_unichar_iswide(ch))
		return 2;

	return 1;
}
#endif /* ENABLE_WIDE */

unsigned
utf8_width(const char *str)
{
	assert(str != NULL);

#if defined(ENABLE_MULTIBYTE) && !defined(ENABLE_WIDE)
	return g_utf8_strlen(str, -1);
#else
#ifdef ENABLE_WIDE
	if (g_utf8_validate(str, -1, NULL)) {
		size_t len = g_utf8_strlen(str, -1);
		unsigned width = 0;
		gunichar c;

		while (len--) {
			c = g_utf8_get_char(str);
			width += unicode_char_width(c);
			str += g_unichar_to_utf8(c, NULL);
		}

		return width;
	} else
#endif
		return strlen(str);
#endif
}

unsigned
locale_width(const char *p)
{
#if defined(ENABLE_LOCALE) && defined(ENABLE_MULTIBYTE)
	char *utf8;
	unsigned width;

	if (noconvert)
		return utf8_width(p);

	utf8 = locale_to_utf8(p);
	width = utf8_width(utf8);
	g_free(utf8);

	return width;
#else
	return strlen(p);
#endif
}

static inline unsigned
ascii_cut_width(char *p, unsigned max_width)
{
	size_t length = strlen(p);
	if (length <= (size_t)max_width)
		return (unsigned)length;

	p[max_width] = 0;
	return max_width;
}

static inline unsigned
narrow_cut_width(char *p, unsigned max_width)
{
	size_t length = g_utf8_strlen(p, -1);
	if (length <= (size_t)max_width)
		return (unsigned)length;

	*g_utf8_offset_to_pointer(p, max_width) = 0;
	return max_width;
}

static inline unsigned
wide_cut_width(char *p, unsigned max_width)
{
	size_t length = g_utf8_strlen(p, -1);
	unsigned width = 0, prev_width;
	gunichar c;

	while (length-- > 0) {
		c = g_utf8_get_char(p);
		prev_width = width;
		width += g_unichar_iswide(c) ? 2 : 1;
		if (width > max_width) {
			/* too wide - cut the rest off */
			*p = 0;
			return prev_width;
		}

		p += g_unichar_to_utf8(c, NULL);
	}

	return width;
}

unsigned
utf8_cut_width(char *p, unsigned max_width)
{
	assert(p != NULL);

#ifdef ENABLE_WIDE
	if (!g_utf8_validate(p, -1, NULL))
		return ascii_cut_width(p, max_width);

	return wide_cut_width(p, max_width);
#elif defined(ENABLE_MULTIBYTE) && !defined(ENABLE_WIDE)
	return narrow_cut_width(p, max_width);
#else
	return ascii_cut_width(p, max_width);
#endif
}

char *
utf8_to_locale(const char *utf8str)
{
#ifdef ENABLE_LOCALE
	gchar *str;

	assert(utf8str != NULL);

	if (noconvert)
		return g_strdup(utf8str);

	str = g_convert_with_fallback(utf8str, -1,
				      charset, "utf-8",
				      NULL, NULL, NULL, NULL);
	if (str == NULL)
		return g_strdup(utf8str);

	return str;
#else
	return g_strdup(utf8str);
#endif
}

char *
locale_to_utf8(const char *localestr)
{
#ifdef ENABLE_LOCALE
	gchar *str;

	assert(localestr != NULL);

	if (noconvert)
		return g_strdup(localestr);

	str = g_convert_with_fallback(localestr, -1,
				      "utf-8", charset,
				      NULL, NULL, NULL, NULL);
	if (str == NULL)
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
	assert(src != NULL);

	if (noconvert)
		return src;

	return utf8_to_locale(src);
#else
	return src;
#endif
}


char *
replace_locale_to_utf8(char *src)
{
#ifdef ENABLE_LOCALE
	assert(src != NULL);

	if (noconvert)
		return src;

	return locale_to_utf8(src);
#else
	return src;
#endif
}
