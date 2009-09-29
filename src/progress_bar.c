/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#include "progress_bar.h"

#include <assert.h>

void
progress_bar_paint(const struct progress_bar *p)
{
	assert(p != NULL);

	mvwhline(p->window.w, 0, 0, ACS_HLINE, p->window.cols);

	if (p->max > 0) {
		assert(p->width < p->window.cols);

		if (p->width > 0)
			whline(p->window.w, '=', p->width);

		mvwaddch(p->window.w, 0, p->width, 'O');
	}

	wnoutrefresh(p->window.w);
}

bool
progress_bar_resize(struct progress_bar *p)
{
	unsigned old_width;

	assert(p != NULL);

	if (p->max == 0)
		return false;

	old_width = p->width;
	p->width = (p->window.cols * p->current) / (p->max + 1);
	assert(p->width < p->window.cols);

	return p->width != old_width;
}

bool
progress_bar_set(struct progress_bar *p, unsigned current, unsigned max)
{
	bool modified;

	assert(p != NULL);

	if (current > max)
		current = max;

	modified = (max == 0) != (p->max == 0);

	p->max = max;
	p->current = current;

	return progress_bar_resize(p) || modified;
}
