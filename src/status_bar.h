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

#ifndef NCMPC_STATUS_BAR_H
#define NCMPC_STATUS_BAR_H

#include "window.h"

#include <glib.h>

#include <stdbool.h>

struct mpd_status;
struct mpd_song;

struct status_bar {
	struct window window;

	GTime message_timestamp;
};

static inline void
status_bar_init(struct status_bar *p, unsigned width, int y, int x)
{
	window_init(&p->window, 1, width, y, x);

	leaveok(p->window.w, false);
	keypad(p->window.w, true);

	p->message_timestamp = 0;
}

static inline void
status_bar_deinit(struct status_bar *p)
{
	delwin(p->window.w);
}

void
status_bar_paint(const struct status_bar *p, const struct mpd_status *status,
		 const struct mpd_song *song);

void
status_bar_resize(struct status_bar *p, unsigned width, int y, int x);

void
status_bar_message(struct status_bar *p, const char *msg);

#endif
