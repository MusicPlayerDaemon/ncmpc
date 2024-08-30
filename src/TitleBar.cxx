// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TitleBar.hxx"
#include "TabBar.hxx"
#include "Styles.hxx"
#include "Options.hxx"
#include "i18n.h"
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
	if (options.enable_colors)
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
		const char *title) const noexcept
{
	WINDOW *w = window.w;

	wmove(w, 0, 0);
	wclrtoeol(w);

#ifndef NCMPC_MINI
	if (options.welcome_screen_list) {
		PaintTabBar(w, current_page_meta, title);
	} else {
#else
		(void)current_page_meta;
#endif
		SelectStyle(w, Style::TITLE_BOLD);
		mvwaddstr(w, 0, 0, title);
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

	SelectStyle(w, Style::TITLE);
	const unsigned window_width = window.GetWidth();
	mvwaddstr(w, 0, window_width - StringWidthMB(volume_string),
		  volume_string);

	SelectStyle(w, Style::LINE);
	mvwhline(w, 1, 0, ACS_HLINE, window_width);
	if (flags[0]) {
		wmove(w, 1, window_width - strlen(flags) - 3);
		waddch(w, '[');
		SelectStyle(w, Style::LINE_FLAGS);
		waddstr(w, flags);
		SelectStyle(w, Style::LINE);
		waddch(w, ']');
	}

	wnoutrefresh(w);
}

void
TitleBar::OnResize(unsigned width) noexcept
{
	window.Resize({width, GetHeight()});
}
