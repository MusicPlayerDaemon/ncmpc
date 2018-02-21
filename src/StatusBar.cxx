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

#include "StatusBar.hxx"
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
StatusBar::Init(unsigned width, int y, int x)
{
	window_init(&window, 1, width, y, x);

	leaveok(window.w, false);
	keypad(window.w, true);

	message_source_id = 0;

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_init(&hscroll, window.w, options.scroll_sep);
#endif

#ifdef ENABLE_COLORS
	if (options.enable_colors)
		wbkgd(window.w, COLOR_PAIR(COLOR_STATUS));
#endif
}

void
StatusBar::Deinit()
{
	delwin(window.w);

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&hscroll);
#endif
}

void
StatusBar::ClearMessage()
{
	if (message_source_id != 0) {
		g_source_remove(message_source_id);
		message_source_id = 0;
	}

	WINDOW *w = window.w;

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
StatusBar::Paint(const struct mpd_status *status,
		 const struct mpd_song *song)
{
	WINDOW *w = window.w;
	char buffer[window.cols * 4 + 1];

	if (message_source_id != 0)
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
		char songname[window.cols * 4 + 1];
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
			hscroll_set(&hscroll, x, 0, width, songname);
			hscroll_draw(&hscroll);
		} else {
			if (options.scroll)
				hscroll_clear(&hscroll);
			mvwaddstr(w, 0, x, songname);
		}
#else
		mvwaddstr(w, 0, x, songname);
#endif
#ifndef NCMPC_MINI
	} else if (options.scroll) {
		hscroll_clear(&hscroll);
#endif
	}

	/* display time string */
	if (buffer[0] != 0) {
		x = window.cols - strlen(buffer);
		colors_use(w, COLOR_STATUS_TIME);
		mvwaddstr(w, 0, x, buffer);
	}

	wnoutrefresh(w);
}

void
StatusBar::OnResize(unsigned width, int y, int x)
{
	window.cols = width;
	wresize(window.w, 1, width);
	mvwin(window.w, y, x);
}

inline void
StatusBar::OnClearMessageTimer()
{
	assert(message_source_id != 0);
	message_source_id = 0;
	ClearMessage();
}

gboolean
StatusBar::OnClearMessageTimer(gpointer data)
{
	auto &sb = *(StatusBar *)data;
	sb.OnClearMessageTimer();
	return false;
}

void
StatusBar::SetMessage(const char *msg)
{
	WINDOW *w = window.w;

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&hscroll);
#endif

	wmove(w, 0, 0);
	wclrtoeol(w);
	colors_use(w, COLOR_STATUS_ALERT);
	waddstr(w, msg);
	wnoutrefresh(w);

	if (message_source_id != 0)
		g_source_remove(message_source_id);
	message_source_id = g_timeout_add_seconds(options.status_message_time,
						  OnClearMessageTimer, this);
}
