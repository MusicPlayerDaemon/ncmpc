/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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
#include "Options.hxx"
#include "Styles.hxx"
#include "i18n.h"
#include "strfsong.hxx"
#include "DelayedSeek.hxx"
#include "time_format.hxx"
#include "util/LocaleString.hxx"

#include <mpd/client.h>

#include <string.h>

StatusBar::StatusBar(boost::asio::io_service &io_service,
		     Point p, unsigned width) noexcept
	:window(p, {width, 1u}),
	 message_timer(io_service)
#ifndef NCMPC_MINI
	, hscroll(io_service, window.w, options.scroll_sep.c_str())
#endif
{
	leaveok(window.w, false);
	keypad(window.w, true);

#ifdef ENABLE_COLORS
	if (options.enable_colors)
		wbkgd(window.w, COLOR_PAIR(Style::STATUS));
#endif
}

StatusBar::~StatusBar() noexcept
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif
}

void
StatusBar::ClearMessage() noexcept
{
	message_timer.cancel();
	message.clear();

	Paint();
	doupdate();
}

static size_t
format_bitrate(char *p, size_t max_length,
	       const struct mpd_status *status) noexcept
{

#ifndef NCMPC_MINI
	if (options.visible_bitrate && mpd_status_get_kbit_rate(status) > 0) {
		snprintf(p, max_length,
			 " [%d kbps]",
			 mpd_status_get_kbit_rate(status));
		return strlen(p);
	} else {
#else
		(void)max_length;
		(void)status;
#endif
		p[0] = '\0';
		return 0;
#ifndef NCMPC_MINI
	}
#endif
}

static void
FormatCurrentSongTime(char *buffer, size_t size,
		      const struct mpd_status &status,
		      const DelayedSeek &seek) noexcept
{
	const unsigned total_time = mpd_status_get_total_time(&status);
	if (total_time == 0)
		return;

	unsigned elapsed_time = seek.IsSeeking(mpd_status_get_song_id(&status))
		? seek.GetTime()
		: mpd_status_get_elapsed_time(&status);
	if (elapsed_time == 0)
		return;

	char elapsed_string[32], duration_string[32];

	/* checks the conf to see whether to display elapsed or
	   remaining time */
	switch (options.current_time_display) {
	case CurrentTimeDisplay::ELAPSED:
		break;

	case CurrentTimeDisplay::REMAINING:
		elapsed_time = elapsed_time < total_time
			? total_time - elapsed_time
			: 0;
		break;
	}

	/* write out the time */
	format_duration_short(elapsed_string,
			      sizeof(elapsed_string),
			      elapsed_time);
	format_duration_short(duration_string,
			      sizeof(duration_string),
			      total_time);

	snprintf(buffer, size, " [%s/%s]", elapsed_string, duration_string);
}

inline size_t
FormatStatusRightText(char *buffer, size_t size,
		      const struct mpd_status &status,
		      const DelayedSeek &seek) noexcept
{
	/* display bitrate if visible-bitrate is true */
	size_t offset = format_bitrate(buffer, size, &status);

	FormatCurrentSongTime(buffer + offset, size - offset, status, seek);

	return StringWidthMB(buffer);
}

void
StatusBar::Update(const struct mpd_status *status,
		  const struct mpd_song *song,
		  const DelayedSeek &seek) noexcept
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
		? StringWidthMB(left_text) + 1
		: 0;

	if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
		right_width = FormatStatusRightText(right_text,
						    sizeof(right_text),
						    *status, seek);

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
		center_width = StringWidthMB(center_text.c_str());
		if (options.scroll && width > 3 &&
		    center_width > (unsigned)width) {
			hscroll.Set(left_width, 0, width, center_text.c_str(),
				    Style::STATUS);
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
StatusBar::Paint() const noexcept
{
	WINDOW *w = window.w;

	wmove(w, 0, 0);
	wclrtoeol(w);

	if (!message.empty()) {
		SelectStyle(w, Style::STATUS_ALERT);
		waddstr(w, message.c_str());
		wnoutrefresh(w);
		return;
	}

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
StatusBar::OnResize(Point p, unsigned width) noexcept
{
	window.Resize({width, 1u});
	window.Move(p);
}

void
StatusBar::SetMessage(const char *msg) noexcept
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif

	message = msg;
	Paint();
	doupdate();

	boost::system::error_code error;
	message_timer.expires_from_now(options.status_message_time,
				       error);
	message_timer.async_wait(std::bind(&StatusBar::OnMessageTimer, this,
					   std::placeholders::_1));
}
