/*
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "charset.h"

#include <assert.h>
#include <string.h>
#include <glib.h>

#ifdef HAVE_LOCALE_H
static bool noconvert = true;
static const char *charset;

const char *
charset_init(void)
{
	noconvert = g_get_charset(&charset);
	return charset;
	return NULL;
}
#endif

unsigned
utf8_width(const char *str)
{
	assert(str != NULL);

#ifdef ENABLE_WIDE
	if (g_utf8_validate(str, -1, NULL)) {
		size_t len = g_utf8_strlen(str, -1);
		unsigned width = 0;
		gunichar c;

		while (len--) {
			c = g_utf8_get_char(str);
			width += g_unichar_iswide(c) ? 2 : 1;
			str += g_unichar_to_utf8(c, NULL);
		}

		return width;
	} else
#endif
		return strlen(str);
}

char *
utf8_to_locale(const char *utf8str)
{
#ifdef HAVE_LOCALE_H
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
#ifdef HAVE_LOCALE_H
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
