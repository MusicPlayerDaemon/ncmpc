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

#include "screen_utils.hxx"
#include "screen.hxx"
#include "mpdclient.hxx"
#include "config.h"
#include "i18n.h"
#include "options.hxx"
#include "colors.hxx"
#include "wreadln.hxx"
#include "ncmpc.hxx"

#include <mpd/client.h>
#include <ctype.h>

void
screen_bell()
{
	if (options.audible_bell)
		beep();
	if (options.visible_bell)
		flash();
}

static bool
ignore_key(int key)
{
	return
#ifdef HAVE_GETMOUSE
		/* ignore mouse events */
		key == KEY_MOUSE ||
#endif
		key == ERR;
}

int
screen_getch(const char *prompt)
{
	WINDOW *w = screen->status_bar.GetWindow().w;

	colors_use(w, COLOR_STATUS_ALERT);
	werase(w);
	wmove(w, 0, 0);
	waddstr(w, prompt);

	echo();
	curs_set(1);

	int key;
	while (ignore_key(key = wgetch(w))) {}

	noecho();
	curs_set(0);

	return key;
}

bool
screen_get_yesno(const char *_prompt, bool def)
{
	/* NOTE: if one day a translator decides to use a multi-byte character
	   for one of the yes/no keys, we'll have to parse it properly */

	char *prompt = g_strdup_printf(_("%s [%s/%s] "), _prompt, YES, NO);
	int key = tolower(screen_getch(prompt));
	g_free(prompt);

	if (key == YES[0])
		return true;
	else if (key == NO[0])
		return false;
	else
		return def;
}

char *
screen_readln(const char *prompt,
	      const char *value,
	      GList **history,
	      GCompletion *gcmp)
{
	struct window *window = &screen->status_bar.GetWindow();
	WINDOW *w = window->w;
	char *line = nullptr;

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);
	line = wreadln(w, prompt, value, window->cols, history, gcmp);
	curs_set(0);
	return line;
}

char *
screen_read_password(const char *prompt)
{
	struct window *window = &screen->status_bar.GetWindow();
	WINDOW *w = window->w;

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);

	if (prompt == nullptr)
		prompt = _("Password");
	char *ret = wreadln_masked(w, prompt, nullptr, window->cols, nullptr, nullptr);

	curs_set(0);
	return ret;
}

void
screen_display_completion_list(GList *list)
{
	static GList *prev_list = nullptr;
	static guint prev_length = 0;
	static guint offset = 0;
	WINDOW *w = screen->main_window.w;

	unsigned length = g_list_length(list);
	if (list == prev_list && length == prev_length) {
		offset += screen->main_window.rows;
		if (offset >= length)
			offset = 0;
	} else {
		prev_list = list;
		prev_length = length;
		offset = 0;
	}

	colors_use(w, COLOR_STATUS_ALERT);

	unsigned y = 0;
	while (y < screen->main_window.rows) {
		GList *item = g_list_nth(list, y+offset);

		wmove(w, y++, 0);
		wclrtoeol(w);
		if (item) {
			gchar *tmp = g_strdup((const char *)item->data);
			waddstr(w, g_basename(tmp));
			g_free(tmp);
		}
	}

	wrefresh(w);
	colors_use(w, COLOR_LIST);
}
