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
	window.Refresh();
	ScheduleTimer();
}

void
hscroll::Set(Point _position, unsigned _width, std::string_view _text,
	     Style _style, attr_t _attr) noexcept
{
	position = _position;
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
	assert(basic.IsDefined());

	SelectStyle(window, style);

	if (attr != 0)
		window.AttributeOn(attr);

	/* scroll the string, and draw it */
	const auto s = basic.ScrollString();
	window.String(position, s);

	if (attr != 0)
		window.AttributeOff(attr);
}
