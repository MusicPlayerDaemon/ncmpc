// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ProgressBar.hxx"
#include "Styles.hxx"
#include "Options.hxx"
#include "config.h"

#include <assert.h>

ProgressBar::ProgressBar(Point p, unsigned _width) noexcept
	:window(p, {_width, 1u})
{
	leaveok(window.w, true);
#ifdef ENABLE_COLORS
	if (options.enable_colors)
		window.SetBackgroundStyle(Style::PROGRESSBAR);
#endif
}

void
ProgressBar::Paint() const noexcept
{
	const unsigned window_width = window.GetWidth();

	if (max > 0) {
		assert(width < window_width);

		SelectStyle(window.w, Style::PROGRESSBAR);

		if (width > 0)
			mvwhline(window.w, 0, 0, '=', width);

		mvwaddch(window.w, 0, width, '>');
		unsigned x = width + 1;

		if (x < window_width) {
			SelectStyle(window.w, Style::PROGRESSBAR_BACKGROUND);
			mvwhline(window.w, 0, x, ACS_HLINE, window_width - x);
		}
	} else {
		/* no progress bar, just a simple horizontal line */
		SelectStyle(window.w, Style::LINE);
		mvwhline(window.w, 0, 0, ACS_HLINE, window_width);
	}

	wnoutrefresh(window.w);
}

bool
ProgressBar::Calculate() noexcept
{
	if (max == 0)
		return false;

	const unsigned window_width = window.GetWidth();
	unsigned old_width = width;
	width = (window_width * current) / (max + 1);
	assert(width < window_width);

	return width != old_width;
}

void
ProgressBar::OnResize(Point p, unsigned _width) noexcept
{
	window.Resize({_width, 1u});
	window.Move(p);

	Calculate();
}

bool
ProgressBar::Set(unsigned _current, unsigned _max) noexcept
{
	if (_current > _max)
		_current = _max;

	bool modified = (_max == 0) != (max == 0);

	max = _max;
	current = _current;

	return Calculate() || modified;
}
