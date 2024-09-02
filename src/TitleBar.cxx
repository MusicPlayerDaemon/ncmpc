// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TitleBar.hxx"
#include "TabBar.hxx"
#include "Styles.hxx"
#include "Options.hxx"
#include "i18n.h"
#include "ui/Options.hxx"
#include "util/LocaleString.hxx"

#include "config.h"

#include <mpd/client.h>

#include <string.h>

TitleBar::TitleBar(Point p, unsigned width) noexcept
	:window(p, {width, GetHeight()})
{
	leaveok(window.w, true);
	keypad(window.w, true);

#ifdef ENABLE_COLORS
	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::TITLE);
#endif
}

static inline int
get_volume(const struct mpd_status *status) noexcept
{
	return status != nullptr
		? mpd_status_get_volume(status)
		: -1;
}

void
TitleBar::Update(const struct mpd_status *status) noexcept
{
	volume = get_volume(status);

	char *p = flags;
	if (status != nullptr) {
		if (mpd_status_get_repeat(status))
			*p++ = 'r';
		if (mpd_status_get_random(status))
			*p++ = 'z';
		if (mpd_status_get_single(status))
			*p++ = 's';
		if (mpd_status_get_consume(status))
			*p++ = 'c';
		if (mpd_status_get_crossfade(status))
			*p++ = 'x';
		if (mpd_status_get_update_id(status) != 0)
			*p++ = 'U';
	}
	*p = 0;
}

void
TitleBar::Paint(const PageMeta &current_page_meta,
		std::string_view title) const noexcept
{
	window.MoveCursor({0, 0});
	window.ClearToEol();

#ifndef NCMPC_MINI
	if (options.welcome_screen_list) {
		PaintTabBar(window, current_page_meta, title);
	} else {
#else
		(void)current_page_meta;
#endif
		SelectStyle(window, Style::TITLE_BOLD);
		window.String({0, 0}, title);
#ifndef NCMPC_MINI
	}
#endif

	char buf[32];
	const char *volume_string;
	if (volume < 0)
		volume_string = _("Volume n/a");
	else {
		snprintf(buf, sizeof(buf), _("Volume %d%%"), volume);
		volume_string = buf;
	}

	SelectStyle(window, Style::TITLE);
	const int window_width = window.GetWidth();
	window.String({window_width - (int)StringWidthMB(volume_string), 0},
		      volume_string);

	SelectStyle(window, Style::LINE);
	window.HLine({0, 1}, window_width, ACS_HLINE);
	if (flags[0]) {
		window.MoveCursor({window_width - (int)strlen(flags) - 3, 1});
		window.Char('[');
		SelectStyle(window, Style::LINE_FLAGS);
		window.String(flags);
		SelectStyle(window, Style::LINE);
		window.Char(']');
	}

	window.RefreshNoOut();
}

void
TitleBar::OnResize(unsigned width) noexcept
{
	window.Resize({width, GetHeight()});
}
