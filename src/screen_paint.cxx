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

#include "screen.hxx"
#include "screen_interface.hxx"
#include "config.h"
#include "mpdclient.hxx"
#include "options.hxx"
#include "player_command.hxx"

#include <mpd/client.h>

#include <assert.h>

void
paint_top_window(const struct mpdclient *c)
{
	const char *title =
#ifndef NCMPC_MINI
		screen.welcome_source_id == 0 &&
#endif
		screen.current_page->get_title != nullptr
		? screen.current_page->get_title(screen.buf, screen.buf_size)
		: "";
	assert(title != nullptr);

	title_bar_paint(&screen.title_bar, title, c->status);
}

static void
update_progress_window(struct mpdclient *c, bool repaint)
{
	unsigned elapsed;
	if (c->status == nullptr)
		elapsed = 0;
	else if (seek_id >= 0 && seek_id == mpd_status_get_song_id(c->status))
		elapsed = seek_target_time;
	else
		elapsed = mpd_status_get_elapsed_time(c->status);

	unsigned duration = mpdclient_is_playing(c)
		? mpd_status_get_total_time(c->status)
		: 0;

	if (progress_bar_set(&screen.progress_bar, elapsed, duration) ||
	    repaint)
		progress_bar_paint(&screen.progress_bar);
}

void
screen_paint(struct mpdclient *c, bool main_dirty)
{
	/* update title/header window */
	paint_top_window(c);

	/* paint the bottom window */

	update_progress_window(c, true);
	status_bar_paint(&screen.status_bar, c->status, c->song);

	/* paint the main window */

	if (main_dirty) {
		wclear(screen.main_window.w);
		if (screen.current_page->paint != nullptr)
			screen.current_page->paint();
	}

	/* move the cursor to the origin */

	if (!options.hardware_cursor)
		wmove(screen.main_window.w, 0, 0);

	wnoutrefresh(screen.main_window.w);

	/* tell curses to update */
	doupdate();
}
