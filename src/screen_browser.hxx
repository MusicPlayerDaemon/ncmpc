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

#ifndef SCREEN_BROWSER_H
#define SCREEN_BROWSER_H

#include "command.hxx"
#include "config.h"
#include "ncmpc_curses.h"

struct mpdclient;
struct mpdclient_playlist;
struct filelist;
struct list_window;
struct list_window_state;

struct screen_browser {
	struct list_window *lw;

	struct filelist *filelist;
	const char *song_format;
};

#ifndef NCMPC_MINI

void
screen_browser_sync_highlights(struct filelist *fl,
			       const struct mpdclient_playlist *playlist);

#else

static inline void
screen_browser_sync_highlights(gcc_unused struct filelist *fl,
			       gcc_unused const struct mpdclient_playlist *playlist)
{
}

#endif

void
screen_browser_paint_directory(WINDOW *w, unsigned width,
			       bool selected, const char *name);

void
screen_browser_paint(const struct screen_browser *browser);

#ifdef HAVE_GETMOUSE
bool
browser_mouse(struct screen_browser *browser,
	      struct mpdclient *c, int x, int y, mmask_t bstate);
#endif

bool
browser_cmd(struct screen_browser *browser,
	    struct mpdclient *c, command_t cmd);

struct filelist_entry *
browser_get_selected_entry(const struct screen_browser *browser);

#endif
