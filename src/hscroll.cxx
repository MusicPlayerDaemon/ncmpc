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

#include "hscroll.hxx"
#include "charset.hxx"
#include "ncfix.h"

#include <assert.h>
#include <string.h>

char *
hscroll::ScrollString(const char *str, const char *_separator,
		      unsigned _width)
{
	assert(str != nullptr);
	assert(_separator != nullptr);

	/* create a buffer containing the string and the separator */
	char *tmp = replace_locale_to_utf8(g_strconcat(str, _separator,
						       str, _separator, nullptr));
	if (offset >= (unsigned)g_utf8_strlen(tmp, -1) / 2)
		offset = 0;

	/* create the new scrolled string */
	char *buf = g_utf8_offset_to_pointer(tmp, offset);
	utf8_cut_width(buf, _width);

	/* convert back to locale */
	buf = utf8_to_locale(buf);
	g_free(tmp);
	return buf;
}

gboolean
hscroll::TimerCallback(gpointer data)
{
	auto &hscroll = *(struct hscroll *)data;
	hscroll.TimerCallback();
	return true;
}

inline void
hscroll::TimerCallback()
{
	Step();
	Paint();
	wrefresh(w);
}

void
hscroll::Set(unsigned _x, unsigned _y, unsigned _width, const char *_text)
{
	assert(w != nullptr);
	assert(_text != nullptr);

	if (text != nullptr && _x == x && _y == y &&
	    _width == width && strcmp(_text, text) == 0)
		/* no change, do nothing (and, most importantly, do
		   not reset the current offset!) */
		return;

	Clear();

	x = _x;
	y = _y;
	width = _width;

	/* obtain the ncurses attributes and the current color, store
	   them */
	fix_wattr_get(w, &attrs, &pair, nullptr);

	text = g_strdup(_text);
	offset = 0;
	source_id = g_timeout_add_seconds(1, TimerCallback, this);
}

void
hscroll::Clear()
{
	if (text == nullptr)
		return;

	g_source_remove(source_id);

	g_free(text);
	text = nullptr;
}

void
hscroll::Paint()
{
	assert(w != nullptr);
	assert(text != nullptr);

	/* set stored attributes and color */
	attr_t old_attrs;
	short old_pair;
	fix_wattr_get(w, &old_attrs, &old_pair, nullptr);
	wattr_set(w, attrs, pair, nullptr);

	/* scroll the string, and draw it */
	char *p = ScrollString(text, separator, width);
	mvwaddstr(w, y, x, p);
	g_free(p);

	/* restore previous attributes and color */
	wattr_set(w, old_attrs, old_pair, nullptr);
}
