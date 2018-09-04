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

#include "TabBar.hxx"
#include "colors.hxx"
#include "command.hxx"
#include "i18n.h"

static void
PaintPageTab(WINDOW *w, command_t cmd, const char *label)
{
	colors_use(w, COLOR_TITLE_BOLD);
	waddstr(w, get_key_names(cmd, false));
	colors_use(w, COLOR_TITLE);
	waddch(w, ':');
	waddstr(w, label);
	waddch(w, ' ');
	waddch(w, ' ');
}

void
PaintTabBar(WINDOW *w)
{
#ifdef ENABLE_HELP_SCREEN
	PaintPageTab(w, CMD_SCREEN_HELP, _("Help"));
#endif
	PaintPageTab(w, CMD_SCREEN_PLAY, _("Queue"));
	PaintPageTab(w, CMD_SCREEN_FILE, _("Browse"));
#ifdef ENABLE_ARTIST_SCREEN
	PaintPageTab(w, CMD_SCREEN_ARTIST, _("Artist"));
#endif
#ifdef ENABLE_SEARCH_SCREEN
	PaintPageTab(w, CMD_SCREEN_SEARCH, _("Search"));
#endif
#ifdef ENABLE_LYRICS_SCREEN
	PaintPageTab(w, CMD_SCREEN_LYRICS, _("Lyrics"));
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	PaintPageTab(w, CMD_SCREEN_OUTPUTS, _("Outputs"));
#endif
#ifdef ENABLE_CHAT_SCREEN
	PaintPageTab(w, CMD_SCREEN_CHAT, _("Chat"));
#endif
}
