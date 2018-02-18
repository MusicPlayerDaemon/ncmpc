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

#ifndef SCREEN_H
#define SCREEN_H

#include "config.h"
#include "command.hxx"
#include "window.hxx"
#include "title_bar.hxx"
#include "progress_bar.hxx"
#include "status_bar.hxx"
#include "ncmpc_curses.h"

#include <mpd/client.h>

#include <glib.h>

struct mpdclient;
struct screen_functions;

class ScreenManager {
public:
	struct title_bar title_bar;
	struct window main_window;
	struct progress_bar progress_bar;
	struct status_bar status_bar;

	const struct screen_functions *current_page;

	char *buf;
	size_t buf_size;

	char *findbuf;
	GList *find_history;

#ifndef NCMPC_MINI
	/**
	 * Non-zero when the welcome message is currently being
	 * displayed.  The associated timer will disable it.
	 */
	guint welcome_source_id;
#endif
};

extern ScreenManager screen;

void screen_init(struct mpdclient *c);
void screen_exit();
void screen_resize(struct mpdclient *c);

void
paint_top_window(const struct mpdclient *c);

void
screen_paint(struct mpdclient *c, bool main_dirty);

void screen_update(struct mpdclient *c);
void screen_cmd(struct mpdclient *c, command_t cmd);

#ifdef HAVE_GETMOUSE
bool
screen_mouse(struct mpdclient *c, int x, int y, mmask_t bstate);
#endif

void
screen_switch(const struct screen_functions *sf, struct mpdclient *c);
void 
screen_swap(struct mpdclient *c, const struct mpd_song *song);

static inline bool
screen_is_visible(const struct screen_functions *sf)
{
	return sf == screen.current_page;
}

#endif
