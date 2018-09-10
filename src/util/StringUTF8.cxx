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

#include "StringUTF8.hxx"
#include "util/ScopeExit.hxx"

#include <glib.h>

#include <assert.h>
#include <string.h>

#ifdef HAVE_CURSES_ENHANCED
static inline unsigned
unicode_char_width(gunichar ch)
{
	if (g_unichar_iszerowidth(ch))
		return 0;

	if (g_unichar_iswide(ch))
		return 2;

	return 1;
}
#endif /* HAVE_CURSES_ENHANCED */

unsigned
utf8_width(const char *str)
{
	assert(str != nullptr);

#if defined(ENABLE_MULTIBYTE) && !defined(HAVE_CURSES_ENHANCED)
	return g_utf8_strlen(str, -1);
#else
#ifdef HAVE_CURSES_ENHANCED
	if (g_utf8_validate(str, -1, nullptr)) {
		size_t len = g_utf8_strlen(str, -1);
		unsigned width = 0;
		gunichar c;

		while (len--) {
			c = g_utf8_get_char(str);
			width += unicode_char_width(c);
			str += g_unichar_to_utf8(c, nullptr);
		}

		return width;
	} else
#endif
		return strlen(str);
#endif
}

gcc_unused
static unsigned
ascii_cut_width(char *p, unsigned max_width)
{
	size_t length = strlen(p);
	if (length <= (size_t)max_width)
		return (unsigned)length;

	p[max_width] = 0;
	return max_width;
}

gcc_unused
static unsigned
narrow_cut_width(char *p, unsigned max_width)
{
	size_t length = g_utf8_strlen(p, -1);
	if (length <= (size_t)max_width)
		return (unsigned)length;

	*g_utf8_offset_to_pointer(p, max_width) = 0;
	return max_width;
}

gcc_unused
static unsigned
wide_cut_width(char *p, unsigned max_width)
{
	size_t length = g_utf8_strlen(p, -1);
	unsigned width = 0, prev_width;

	while (length-- > 0) {
		gunichar c = g_utf8_get_char(p);
		prev_width = width;
		width += g_unichar_iswide(c) ? 2 : 1;
		if (width > max_width) {
			/* too wide - cut the rest off */
			*p = 0;
			return prev_width;
		}

		p += g_unichar_to_utf8(c, nullptr);
	}

	return width;
}

unsigned
utf8_cut_width(char *p, unsigned max_width)
{
	assert(p != nullptr);

#ifdef HAVE_CURSES_ENHANCED
	if (!g_utf8_validate(p, -1, nullptr))
		return ascii_cut_width(p, max_width);

	return wide_cut_width(p, max_width);
#elif defined(ENABLE_MULTIBYTE) && !defined(HAVE_CURSES_ENHANCED)
	return narrow_cut_width(p, max_width);
#else
	return ascii_cut_width(p, max_width);
#endif
}
