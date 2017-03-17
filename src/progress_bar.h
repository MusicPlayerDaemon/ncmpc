/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#ifndef NCMPC_PROGRESS_BAR_H
#define NCMPC_PROGRESS_BAR_H

#include "window.h"

#include <stdbool.h>

struct progress_bar {
	struct window window;

	unsigned current, max;

	unsigned width;
};

static inline void
progress_bar_init(struct progress_bar *p, unsigned width, int y, int x)
{
	window_init(&p->window, 1, width, y, x);
	leaveok(p->window.w, TRUE);

	p->current = 0;
	p->max = 0;
	p->width = 0;
}

static inline void
progress_bar_deinit(struct progress_bar *p)
{
	delwin(p->window.w);
}

void
progress_bar_paint(const struct progress_bar *p);

void
progress_bar_resize(struct progress_bar *p, unsigned width, int y, int x);

bool
progress_bar_set(struct progress_bar *p, unsigned current, unsigned max);

#endif
