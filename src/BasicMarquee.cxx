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

#include "BasicMarquee.hxx"
#include "charset.hxx"

#include <glib.h>

#include <assert.h>
#include <string.h>

char *
BasicMarquee::ScrollString()
{
	assert(text != nullptr);
	assert(separator != nullptr);

	/* create a buffer containing the string and the separator */
	char *tmp = replace_locale_to_utf8(g_strconcat(text, separator,
						       text, separator, nullptr));
	if (offset >= (unsigned)g_utf8_strlen(tmp, -1) / 2)
		offset = 0;

	/* create the new scrolled string */
	char *buf = g_utf8_offset_to_pointer(tmp, offset);
	utf8_cut_width(buf, width);

	/* convert back to locale */
	buf = utf8_to_locale(buf);
	g_free(tmp);
	return buf;
}

bool
BasicMarquee::Set(unsigned _width, const char *_text)
{
	assert(separator != nullptr);
	assert(_text != nullptr);

	if (text != nullptr && _width == width && strcmp(_text, text) == 0)
		/* no change, do nothing (and, most importantly, do
		   not reset the current offset!) */
		return false;

	Clear();

	width = _width;

	text = g_strdup(_text);
	offset = 0;

	return true;
}

void
BasicMarquee::Clear()
{
	g_free(text);
	text = nullptr;
}
