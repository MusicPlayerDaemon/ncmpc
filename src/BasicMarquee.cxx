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
#include "util/ScopeExit.hxx"
#include "util/StringUTF8.hxx"

#include <glib.h>

#include <algorithm>

#include <assert.h>
#include <string.h>

char *
BasicMarquee::ScrollString() const
{
	assert(text != nullptr);
	assert(separator != nullptr);

	/* create the new scrolled string */
	char *tmp = g_strdup(g_utf8_offset_to_pointer(text_utf8, offset));
	AtScopeExit(tmp) { g_free(tmp); };

	utf8_cut_width(tmp, width);

	/* convert back to locale */
	return utf8_to_locale(tmp);
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
	offset = 0;

	text = g_strdup(_text);

	/* create a buffer containing the string and the separator */
	text_utf8 = replace_locale_to_utf8(g_strconcat(text, separator,
						       text, separator,
						       nullptr));
	text_utf8_length = g_utf8_strlen(text_utf8, -1);

	return true;
}

void
BasicMarquee::Clear()
{
	g_free(text);
	text = nullptr;

	g_free(std::exchange(text_utf8, nullptr));
}
