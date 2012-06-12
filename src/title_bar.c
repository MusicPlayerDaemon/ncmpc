/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#include "title_bar.h"
#include "colors.h"
#include "command.h"
#include "i18n.h"
#include "charset.h"

#include "config.h"

#include <mpd/client.h>

#include <glib.h>

#include <assert.h>
#include <string.h>

#ifndef NCMPC_MINI
static void
print_hotkey(WINDOW *w, command_t cmd, const char *label)
{
	colors_use(w, COLOR_TITLE_BOLD);
	waddstr(w, get_key_names(cmd, false));
	colors_use(w, COLOR_TITLE);
	waddch(w, ':');
	waddstr(w, label);
	waddch(w, ' ');
	waddch(w, ' ');
}
#endif

static inline int
get_volume(const struct mpd_status *status)
{
	return status != NULL
		? mpd_status_get_volume(status)
		: -1;
}

void
title_bar_paint(const struct title_bar *p, const char *title,
		const struct mpd_status *status)
{
	WINDOW *w = p->window.w;
	int volume;
	char flags[5];
	char buf[32];

	assert(p != NULL);

	wmove(w, 0, 0);
	wclrtoeol(w);

	if (title[0]) {
		colors_use(w, COLOR_TITLE_BOLD);
		mvwaddstr(w, 0, 0, title);
#ifndef NCMPC_MINI
	} else {
#ifdef ENABLE_HELP_SCREEN
		print_hotkey(w, CMD_SCREEN_HELP, _("Help"));
#endif
		print_hotkey(w, CMD_SCREEN_PLAY, _("Playlist"));
		print_hotkey(w, CMD_SCREEN_FILE, _("Browse"));
#ifdef ENABLE_ARTIST_SCREEN
		print_hotkey(w, CMD_SCREEN_ARTIST, _("Artist"));
#endif
#ifdef ENABLE_SEARCH_SCREEN
		print_hotkey(w, CMD_SCREEN_SEARCH, _("Search"));
#endif
#ifdef ENABLE_LYRICS_SCREEN
		print_hotkey(w, CMD_SCREEN_LYRICS, _("Lyrics"));
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
		print_hotkey(w, CMD_SCREEN_OUTPUTS, _("Outputs"));
#endif
#ifdef ENABLE_CHAT_SCREEN
		print_hotkey(w, CMD_SCREEN_CHAT, _("Chat"));
#endif
#endif
	}

	volume = get_volume(status);
	if (volume < 0)
		g_snprintf(buf, 32, _("Volume n/a"));
	else
		g_snprintf(buf, 32, _("Volume %d%%"), volume);

	colors_use(w, COLOR_TITLE);
	mvwaddstr(w, 0, p->window.cols - utf8_width(buf), buf);

	flags[0] = 0;
	if (status != NULL) {
		if (mpd_status_get_repeat(status))
			g_strlcat(flags, "r", sizeof(flags));
		if (mpd_status_get_random(status))
			g_strlcat(flags, "z", sizeof(flags));
		if (mpd_status_get_single(status))
			g_strlcat(flags, "s", sizeof(flags));
		if (mpd_status_get_consume(status))
			g_strlcat(flags, "c", sizeof(flags));
		if (mpd_status_get_crossfade(status))
			g_strlcat(flags, "x", sizeof(flags));
		if (mpd_status_get_update_id(status) != 0)
			g_strlcat(flags, "U", sizeof(flags));
	}

	colors_use(w, COLOR_LINE);
	mvwhline(w, 1, 0, ACS_HLINE, p->window.cols);
	if (flags[0]) {
		wmove(w, 1, p->window.cols - strlen(flags) - 3);
		waddch(w, '[');
		colors_use(w, COLOR_LINE_BOLD);
		waddstr(w, flags);
		colors_use(w, COLOR_LINE);
		waddch(w, ']');
	}

	wnoutrefresh(w);
}

void
title_bar_resize(struct title_bar *p, unsigned width)
{
	assert(p != NULL);

	p->window.cols = width;
	wresize(p->window.w, 2, width);
}
