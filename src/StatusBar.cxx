// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "StatusBar.hxx"
#include "Options.hxx"
#include "Styles.hxx"
#include "i18n.h"
#include "strfsong.hxx"
#include "DelayedSeek.hxx"
#include "time_format.hxx"
#include "ui/Options.hxx"
#include "lib/fmt/ToSpan.hxx"
#include "util/LocaleString.hxx"

#include <mpd/client.h>

#include <string.h>

using std::string_view_literals::operator""sv;

StatusBar::StatusBar(EventLoop &event_loop,
		     Point p, unsigned width) noexcept
	:window(p, {width, 1u}),
	 message_timer(event_loop, BIND_THIS_METHOD(OnMessageTimer))
#ifndef NCMPC_MINI
	, hscroll(event_loop, window, options.scroll_sep)
#endif
{
	leaveok(window.w, false);
	keypad(window.w, true);

	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::STATUS);
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
	message_timer.Cancel();
	message.clear();

	Paint();
	doupdate();
}

[[nodiscard]]
static std::string_view 
format_bitrate(std::span<char> buffer,
	       const struct mpd_status *status) noexcept
{

#ifndef NCMPC_MINI
	if (options.visible_bitrate && mpd_status_get_kbit_rate(status) > 0) {
		return FmtTruncate(buffer, " [{} kbps]",
				   mpd_status_get_kbit_rate(status));
	} else {
#else
		(void)buffer;
		(void)status;
#endif
		return {};
#ifndef NCMPC_MINI
	}
#endif
}

[[nodiscard]]
static std::string_view
FormatCurrentSongTime(std::span<char> buffer,
		      const struct mpd_status &status,
		      const DelayedSeek &seek) noexcept
{
	if (options.current_time_display == CurrentTimeDisplay::NONE)
		return {};

	const unsigned total_time = mpd_status_get_total_time(&status);

	unsigned elapsed_time = seek.IsSeeking(mpd_status_get_song_id(&status))
		? seek.GetTime()
		: mpd_status_get_elapsed_time(&status);

	char elapsed_buffer[32], duration_buffer[32];

	/* checks the conf to see whether to display elapsed or
	   remaining time */
	switch (options.current_time_display) {
	case CurrentTimeDisplay::NONE:
	case CurrentTimeDisplay::ELAPSED:
		break;

	case CurrentTimeDisplay::REMAINING:
		if (total_time == 0)
			return {};

		elapsed_time = elapsed_time < total_time
			? total_time - elapsed_time
			: 0;
		break;
	}

	/* write out the time */
	const auto elapsed_string = format_duration_short(elapsed_buffer, elapsed_time);

	if (total_time == 0)
		return FmtTruncate(buffer, " [{}]"sv, elapsed_string);

	const auto duration_string = format_duration_short(duration_buffer, total_time);
	return FmtTruncate(buffer, " [{}/{}]"sv, elapsed_string, duration_string);
}

[[nodiscard]]
static std::string_view
FormatStatusRightText(std::span<char> buffer,
		      const struct mpd_status &status,
		      const DelayedSeek &seek) noexcept
{
	/* display bitrate if visible-bitrate is true */
	const auto bitrate_string = format_bitrate(buffer, &status);

	const auto time_string = FormatCurrentSongTime(buffer.subspan(bitrate_string.size()), status, seek);

	return {buffer.data(), bitrate_string.size() + time_string.size()};
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
		const auto right_text = FormatStatusRightText(right_buffer,
							      *status, seek);
		right_length = right_text.size();
		right_width = StringWidthMB(right_text);

#ifndef NCMPC_MINI
		int width = COLS - left_width - right_width;
#endif

		if (song) {
			char buffer[1024];
			center_text = strfsong(buffer, options.status_format.c_str(), *song);
		} else
			center_text.clear();

		/* scroll if the song name is to long */
#ifndef NCMPC_MINI
		if (options.scroll) {
			const unsigned center_width =
				StringWidthMB(center_text);
			if (width > 3 && center_width > (unsigned)width) {
				hscroll.Set({(int)left_width, 0}, width,
					    center_text,
					    Style::STATUS);
			} else {
				hscroll.Clear();
			}
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
	const unsigned window_width = window.GetWidth();

	window.MoveCursor({0, 0});
	window.ClearToEol();

	if (!message.empty()) {
		SelectStyle(window, Style::STATUS_ALERT);
		window.String(message);
		window.RefreshNoOut();
		return;
	}

	SelectStyle(window, Style::STATUS_BOLD);

	if (left_text != nullptr)
		/* display state */
		window.String(left_text);

	if (right_width > 0) {
		/* display time string */
		int x = window_width - right_width;
		SelectStyle(window, Style::STATUS_TIME);
		window.String({x, 0}, {right_buffer, right_length});
	}

	if (!center_text.empty()) {
		/* display song name */

		SelectStyle(window, Style::STATUS);

		/* scroll if the song name is to long */
#ifndef NCMPC_MINI
		if (hscroll.IsDefined())
			hscroll.Paint();
		else
#endif
			window.String({(int)left_width, 0}, center_text);
	}

	window.RefreshNoOut();
}

void
StatusBar::OnResize(Point p, unsigned width) noexcept
{
	window.Resize({width, 1u});
	window.Move(p);
}

void
StatusBar::SetMessage(std::string &&msg) noexcept
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif

	message = std::move(msg);
	Paint();
	doupdate();

	message_timer.Schedule(options.status_message_time);
}
