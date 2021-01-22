/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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
#include "Styles.hxx"

#include <assert.h>

void
hscroll::OnTimer() noexcept
{
	Step();
	Paint();
	wrefresh(w);
	ScheduleTimer();
}

void
hscroll::Set(unsigned _x, unsigned _y, unsigned _width, const char *_text,
	     Style _style, attr_t _attr) noexcept
{
	assert(w != nullptr);
	assert(_text != nullptr);

	x = _x;
	y = _y;
	style = _style;
	attr = _attr;

	if (!basic.Set(_width, _text))
		return;

	ScheduleTimer();
}

void
hscroll::Clear() noexcept
{
	basic.Clear();
	timer.Cancel();
}

void
hscroll::Paint() const noexcept
{
	assert(w != nullptr);
	assert(basic.IsDefined());

	SelectStyle(w, style);

	if (attr != 0)
		wattron(w, attr);

	/* scroll the string, and draw it */
	const auto s = basic.ScrollString();
	mvwaddnstr(w, y, x, s.first, s.second);

	if (attr != 0)
		wattroff(w, attr);
}
