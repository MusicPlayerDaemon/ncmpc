// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
