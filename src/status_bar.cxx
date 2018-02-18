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

#include "status_bar.hxx"
#include "options.hxx"
#include "colors.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "strfsong.hxx"
#include "player_command.hxx"
#include "time_format.hxx"

#include <mpd/client.h>

#include <assert.h>
#include <string.h>

void
status_bar_init(struct status_bar *p, unsigned width, int y, int x)
{
	window_init(&p->window, 1, width, y, x);

	leaveok(p->window.w, false);
	keypad(p->window.w, true);

	p->message_source_id = 0;

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_init(&p->hscroll, p->window.w, options.scroll_sep);

	p->prev_status = nullptr;
	p->prev_song = nullptr;
#endif
}

void
status_bar_deinit(struct status_bar *p)
{
	delwin(p->window.w);

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&p->hscroll);
#endif
}

void
status_bar_clear_message(struct status_bar *p)
{
	assert(p != nullptr);

	if (p->message_source_id != 0) {
		g_source_remove(p->message_source_id);
		p->message_source_id = 0;
	}

	WINDOW *w = p->window.w;

	wmove(w, 0, 0);
	wclrtoeol(w);
	wrefresh(w);
}

#ifndef NCMPC_MINI

static void
format_bitrate(char *p, size_t max_length, const struct mpd_status *status)
{
	if (options.visible_bitrate && mpd_status_get_kbit_rate(status) > 0)
		g_snprintf(p, max_length,
			   " [%d kbps]",
			   mpd_status_get_kbit_rate(status));
	else
		p[0] = '\0';
}

#endif /* !NCMPC_MINI */

void
status_bar_paint(struct status_bar *p, const struct mpd_status *status,
		 const struct mpd_song *song)
{
	WINDOW *w = p->window.w;
	char buffer[p->window.cols * 4 + 1];

#ifndef NCMPC_MINI
	p->prev_status = status;
	p->prev_song = song;
#endif

	if (p->message_source_id != 0)
		return;

	wmove(w, 0, 0);
	wclrtoeol(w);
	colors_use(w, COLOR_STATUS_BOLD);

	enum mpd_state state = status == nullptr ? MPD_STATE_UNKNOWN
		: mpd_status_get_state(status);

	const char *str = nullptr;
	switch (state) {
	case MPD_STATE_PLAY:
		str = _("Playing:");
		break;
	case MPD_STATE_PAUSE:
		str = _("[Paused]");
		break;
	case MPD_STATE_STOP:
	default:
		break;
	}

	int x = 0;
	if (str) {
		waddstr(w, str);
		x += utf8_width(str) + 1;
	}

	/* create time string */
	if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
		int elapsedTime = seek_id >= 0 &&
			seek_id == mpd_status_get_song_id(status)
			? (unsigned)seek_target_time
			: mpd_status_get_elapsed_time(status);
		int total_time = mpd_status_get_total_time(status);
		if (elapsedTime > 0 || total_time > 0) {
#ifdef NCMPC_MINI
			static char bitrate[1];
#else
			char bitrate[16];
#endif
			char elapsed_string[32], duration_string[32];

			/*checks the conf to see whether to display elapsed or remaining time */
			if (options.display_remaining_time)
				elapsedTime = elapsedTime < total_time
					? total_time - elapsedTime
					: 0;

			/* display bitrate if visible-bitrate is true */
#ifndef NCMPC_MINI
			format_bitrate(bitrate, sizeof(bitrate), status);
#endif

			/* write out the time */
			format_duration_short(elapsed_string,
					      sizeof(elapsed_string),
					      elapsedTime);
			format_duration_short(duration_string,
					      sizeof(duration_string),
					      total_time);

			g_snprintf(buffer, sizeof(buffer), "%s [%s/%s]",
				   bitrate, elapsed_string, duration_string);
#ifndef NCMPC_MINI
		} else {
			format_bitrate(buffer, sizeof(buffer), status);
#else
			buffer[0] = 0;
#endif
		}
	} else {
		buffer[0] = 0;
	}

	/* display song */
	if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
		char songname[p->window.cols * 4 + 1];
#ifndef NCMPC_MINI
		int width = COLS - x - utf8_width(buffer);
#endif

		if (song)
			strfsong(songname, sizeof(songname),
				 options.status_format, song);
		else
			songname[0] = '\0';

		colors_use(w, COLOR_STATUS);
		/* scroll if the song name is to long */
#ifndef NCMPC_MINI
		if (options.scroll && utf8_width(songname) > (unsigned)width) {
			hscroll_set(&p->hscroll, x, 0, width, songname);
			hscroll_draw(&p->hscroll);
		} else {
			if (options.scroll)
				hscroll_clear(&p->hscroll);
			mvwaddstr(w, 0, x, songname);
		}
#else
		mvwaddstr(w, 0, x, songname);
#endif
#ifndef NCMPC_MINI
	} else if (options.scroll) {
		hscroll_clear(&p->hscroll);
#endif
	}

	/* display time string */
	if (buffer[0] != 0) {
		x = p->window.cols - strlen(buffer);
		colors_use(w, COLOR_STATUS_TIME);
		mvwaddstr(w, 0, x, buffer);
	}

	wnoutrefresh(w);
}

void
status_bar_resize(struct status_bar *p, unsigned width, int y, int x)
{
	p->window.cols = width;
	wresize(p->window.w, 1, width);
	mvwin(p->window.w, y, x);
}

static gboolean
status_bar_clear_message_cb(gpointer data)
{
	auto *p = (struct status_bar *)data;
	assert(p->message_source_id != 0);
	p->message_source_id = 0;

	status_bar_clear_message(p);
	return false;
}

void
status_bar_message(struct status_bar *p, const char *msg)
{
	WINDOW *w = p->window.w;

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&p->hscroll);
#endif

	wmove(w, 0, 0);
	wclrtoeol(w);
	colors_use(w, COLOR_STATUS_ALERT);
	waddstr(w, msg);
	wnoutrefresh(w);

	if (p->message_source_id != 0)
		g_source_remove(p->message_source_id);
	p->message_source_id = g_timeout_add_seconds(options.status_message_time,
						     status_bar_clear_message_cb, p);
}
