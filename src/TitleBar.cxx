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

#include "TitleBar.hxx"
#include "TabBar.hxx"
#include "colors.hxx"
#include "options.hxx"
#include "i18n.h"
#include "util/StringUTF8.hxx"

#include "config.h"

#include <mpd/client.h>

#include <string.h>

TitleBar::TitleBar(Point p, unsigned width)
	:window(p, {width, GetHeight()})
{
	leaveok(window.w, true);
	keypad(window.w, true);

#ifdef ENABLE_COLORS
	if (options.enable_colors)
		wbkgd(window.w, COLOR_PAIR(COLOR_TITLE));
#endif
}

static inline int
get_volume(const struct mpd_status *status)
{
	return status != nullptr
		? mpd_status_get_volume(status)
		: -1;
}

void
TitleBar::Update(const struct mpd_status *status)
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
TitleBar::Paint(const PageMeta &current_page_meta, const char *title) const
{
	WINDOW *w = window.w;

	wmove(w, 0, 0);
	wclrtoeol(w);

	if (title[0]) {
		colors_use(w, COLOR_TITLE_BOLD);
		mvwaddstr(w, 0, 0, title);
#ifndef NCMPC_MINI
	} else {
		PaintTabBar(w, current_page_meta);
#else
		(void)current_page_meta;
#endif
	}

	char buf[32];
	const char *volume_string;
	if (volume < 0)
		volume_string = _("Volume n/a");
	else {
		snprintf(buf, sizeof(buf), _("Volume %d%%"), volume);
		volume_string = buf;
	}

	colors_use(w, COLOR_TITLE);
	mvwaddstr(w, 0, window.size.width - utf8_width(volume_string),
		  volume_string);

	colors_use(w, COLOR_LINE);
	mvwhline(w, 1, 0, ACS_HLINE, window.size.width);
	if (flags[0]) {
		wmove(w, 1, window.size.width - strlen(flags) - 3);
		waddch(w, '[');
		colors_use(w, COLOR_LINE_FLAGS);
		waddstr(w, flags);
		colors_use(w, COLOR_LINE);
		waddch(w, ']');
	}

	wnoutrefresh(w);
}

void
TitleBar::OnResize(unsigned width)
{
	window.Resize({width, GetHeight()});
}
