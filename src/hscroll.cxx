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
#include "Event.hxx"

#include <algorithm>

#include <assert.h>

inline bool
hscroll::TimerCallback()
{
	Step();
	Paint();
	wrefresh(w);
	return true;
}

void
hscroll::Set(unsigned _x, unsigned _y, unsigned _width, const char *_text)
{
	assert(w != nullptr);
	assert(_text != nullptr);

	x = _x;
	y = _y;

	if (!basic.Set(_width, _text))
		return;

	/* obtain the ncurses attributes and the current color, store
	   them */
	fix_wattr_get(w, &attrs, &pair, nullptr);

	if (source_id == 0)
		source_id = ScheduleTimeout<hscroll, &hscroll::TimerCallback>(std::chrono::seconds(1), *this);
}

void
hscroll::Clear()
{
	basic.Clear();

	if (source_id != 0)
		g_source_remove(std::exchange(source_id, 0));
}

void
hscroll::Paint() const
{
	assert(w != nullptr);
	assert(basic.IsDefined());

	/* set stored attributes and color */
	wattr_set(w, attrs, pair, nullptr);

	/* scroll the string, and draw it */
	char *p = basic.ScrollString();
	mvwaddstr(w, y, x, p);
	g_free(p);
}
