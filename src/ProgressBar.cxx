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

#include <assert.h>

void
progress_bar_paint(const ProgressBar *p)
{
	assert(p != nullptr);

	mvwhline(p->window.w, 0, 0, ACS_HLINE, p->window.cols);

	if (p->max > 0) {
		assert(p->width < p->window.cols);

		if (p->width > 0)
			whline(p->window.w, '=', p->width);

		mvwaddch(p->window.w, 0, p->width, 'O');
	}

	wnoutrefresh(p->window.w);
}

static bool
progress_bar_calc(ProgressBar *p)
{
	if (p->max == 0)
		return false;

	unsigned old_width = p->width;
	p->width = (p->window.cols * p->current) / (p->max + 1);
	assert(p->width < p->window.cols);

	return p->width != old_width;
}

void
progress_bar_resize(ProgressBar *p, unsigned width, int y, int x)
{
	assert(p != nullptr);

	p->window.cols = width;
	wresize(p->window.w, 1, width);
	mvwin(p->window.w, y, x);

	progress_bar_calc(p);
}

bool
progress_bar_set(ProgressBar *p, unsigned current, unsigned max)
{
	assert(p != nullptr);

	if (current > max)
		current = max;

	bool modified = (max == 0) != (p->max == 0);

	p->max = max;
	p->current = current;

	return progress_bar_calc(p) || modified;
}
