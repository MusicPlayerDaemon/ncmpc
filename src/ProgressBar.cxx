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

#include "ProgressBar.hxx"
#include "colors.hxx"

#include <assert.h>

ProgressBar::ProgressBar(Point p, unsigned _width)
	:window(p, {_width, 1u})
{
	leaveok(window.w, true);
	wbkgd(window.w, COLOR_PAIR(COLOR_PROGRESSBAR));
	colors_use(window.w, COLOR_PROGRESSBAR);
}

void
ProgressBar::Paint() const
{
	mvwhline(window.w, 0, 0, ACS_HLINE, window.size.width);

	if (max > 0) {
		assert(width < window.size.width);

		if (width > 0)
			whline(window.w, '=', width);

		mvwaddch(window.w, 0, width, 'O');
	}

	wnoutrefresh(window.w);
}

bool
ProgressBar::Calculate()
{
	if (max == 0)
		return false;

	unsigned old_width = width;
	width = (window.size.width * current) / (max + 1);
	assert(width < window.size.width);

	return width != old_width;
}

void
ProgressBar::OnResize(Point p, unsigned _width)
{
	window.Resize({_width, 1u});
	window.Move(p);

	Calculate();
}

bool
ProgressBar::Set(unsigned _current, unsigned _max)
{
	if (_current > _max)
		_current = _max;

	bool modified = (_max == 0) != (max == 0);

	max = _max;
	current = _current;

	return Calculate() || modified;
}
