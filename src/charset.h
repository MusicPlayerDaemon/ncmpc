/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#ifndef CHARSET_H
#define CHARSET_H

#include "config.h"

#include <glib.h>

#include <stdbool.h>

#ifdef ENABLE_LOCALE
const char *
charset_init(void);
#endif

/**
 * Returns the number of terminal cells occupied by this string.
 */
G_GNUC_PURE
unsigned
utf8_width(const char *str);

/**
 * Limits the width of the specified string.  Cuts it off before the
 * specified width is exceeded.
 *
 * @return the resulting width of the string
 */
unsigned
utf8_cut_width(char *p, unsigned max_width);

char *utf8_to_locale(const char *str);
char *locale_to_utf8(const char *str);

/**
 * Converts the UTF-8 string to the locale, and frees the source
 * pointer.  Returns the source pointer as-is if no conversion is
 * required.
 */
char *
replace_utf8_to_locale(char *src);

/**
 * Converts the locale string to UTF-8, and frees the source pointer.
 * Returns the source pointer as-is if no conversion is required.
 */
char *
replace_locale_to_utf8(char *src);

#endif
