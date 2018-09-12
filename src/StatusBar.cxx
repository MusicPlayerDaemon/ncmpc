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
#include "Event.hxx"
#include "options.hxx"
#include "Styles.hxx"
#include "i18n.h"
#include "strfsong.hxx"
#include "player_command.hxx"
#include "time_format.hxx"
#include "util/StringUTF8.hxx"

#include <mpd/client.h>

#include <assert.h>
#include <string.h>

StatusBar::StatusBar(Point p, unsigned width)
	:window(p, {width, 1u})
#ifndef NCMPC_MINI
	, hscroll(window.w, options.scroll_sep.c_str())
#endif
{

	leaveok(window.w, false);
	keypad(window.w, true);

#ifdef ENABLE_COLORS
	if (options.enable_colors)
		wbkgd(window.w, COLOR_PAIR(Style::STATUS));
#endif
}

StatusBar::~StatusBar()
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
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
		snprintf(p, max_length,
			 " [%d kbps]",
			 mpd_status_get_kbit_rate(status));
	else
		p[0] = '\0';
}

#endif /* !NCMPC_MINI */

void
StatusBar::Update(const struct mpd_status *status,
		  const struct mpd_song *song)
{
	const auto state = status == nullptr
		? MPD_STATE_UNKNOWN
		: mpd_status_get_state(status);

	left_text = nullptr;
	switch (state) {
	case MPD_STATE_UNKNOWN:
	case MPD_STATE_STOP:
		break;

	case MPD_STATE_PLAY:
		left_text = _("Playing:");
		break;

	case MPD_STATE_PAUSE:
		left_text = _("[Paused]");
		break;
	}

	left_width = left_text != nullptr
		? utf8_width(left_text) + 1
		: 0;

	if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
		unsigned elapsed_time = seek_id >= 0 &&
			seek_id == mpd_status_get_song_id(status)
			? (unsigned)seek_target_time
			: mpd_status_get_elapsed_time(status);
		const unsigned total_time = mpd_status_get_total_time(status);

		if (elapsed_time > 0 || total_time > 0) {
#ifdef NCMPC_MINI
			static char bitrate[1];
#else
			char bitrate[16];
#endif
			char elapsed_string[32], duration_string[32];

			/*checks the conf to see whether to display elapsed or remaining time */
			if (options.display_remaining_time)
				elapsed_time = elapsed_time < total_time
					? total_time - elapsed_time
					: 0;

			/* display bitrate if visible-bitrate is true */
#ifndef NCMPC_MINI
			format_bitrate(bitrate, sizeof(bitrate), status);
#endif

			/* write out the time */
			format_duration_short(elapsed_string,
					      sizeof(elapsed_string),
					      elapsed_time);
			format_duration_short(duration_string,
					      sizeof(duration_string),
					      total_time);

			snprintf(right_text, sizeof(right_text),
				 "%s [%s/%s]",
				 bitrate, elapsed_string, duration_string);
		} else {
#ifndef NCMPC_MINI
			format_bitrate(right_text, sizeof(right_text), status);
#else
			right_text[0] = 0;
#endif
		}

		right_width = utf8_width(right_text);

#ifndef NCMPC_MINI
		int width = COLS - left_width - right_width;
#endif

		if (song) {
			char buffer[1024];
			strfsong(buffer, sizeof(buffer),
				 options.status_format.c_str(), song);
			center_text = buffer;
		} else
			center_text.clear();

		/* scroll if the song name is to long */
#ifndef NCMPC_MINI
		center_width = utf8_width(center_text.c_str());
		if (options.scroll &&
		    utf8_width(center_text.c_str()) > (unsigned)width) {
			hscroll.Set(left_width, 0, width, center_text.c_str());
		} else {
			if (options.scroll)
				hscroll.Clear();
		}
#endif
	} else {
		right_width = 0;
		center_text.clear();

#ifndef NCMPC_MINI
		if (options.scroll)
			hscroll.Clear();
#endif
	}

}

void
StatusBar::Paint() const
{
	WINDOW *w = window.w;

	if (message_source_id != 0)
		return;

	wmove(w, 0, 0);
	wclrtoeol(w);
	SelectStyle(w, Style::STATUS_BOLD);

	if (left_text != nullptr)
		/* display state */
		waddstr(w, left_text);

	if (right_width > 0) {
		/* display time string */
		int x = window.size.width - right_width;
		SelectStyle(w, Style::STATUS_TIME);
		mvwaddstr(w, 0, x, right_text);
	}

	if (!center_text.empty()) {
		/* display song name */

		SelectStyle(w, Style::STATUS);

		/* scroll if the song name is to long */
#ifndef NCMPC_MINI
		if (hscroll.IsDefined())
			hscroll.Paint();
		else
#endif
			mvwaddstr(w, 0, left_width, center_text.c_str());
	}

	/* display time string */
	int x = window.size.width - right_width;
	SelectStyle(w, Style::STATUS_TIME);
	mvwaddstr(w, 0, x, right_text);

	wnoutrefresh(w);
}

void
StatusBar::OnResize(Point p, unsigned width)
{
	window.Resize({width, 1u});
	window.Move(p);
}

inline bool
StatusBar::OnClearMessageTimer()
{
	assert(message_source_id != 0);
	message_source_id = 0;
	ClearMessage();
	return false;
}

void
StatusBar::SetMessage(const char *msg)
{
	WINDOW *w = window.w;

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif

	wmove(w, 0, 0);
	wclrtoeol(w);
	SelectStyle(w, Style::STATUS_ALERT);
	waddstr(w, msg);
	wnoutrefresh(w);

	if (message_source_id != 0)
		g_source_remove(message_source_id);
	message_source_id = ScheduleTimeout<StatusBar,
					    &StatusBar::OnClearMessageTimer>(options.status_message_time,
									     *this);
}
