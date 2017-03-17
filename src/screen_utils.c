/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "screen_utils.h"
#include "screen.h"
#include "mpdclient.h"
#include "config.h"
#include "i18n.h"
#include "options.h"
#include "colors.h"
#include "wreadln.h"

#include <mpd/client.h>
#include <ctype.h>

void
screen_bell(void)
{
	if (options.audible_bell)
		beep();
	if (options.visible_bell)
		flash();
}

int
screen_getch(const char *prompt)
{
	WINDOW *w = screen.status_bar.window.w;

	colors_use(w, COLOR_STATUS_ALERT);
	werase(w);
	wmove(w, 0, 0);
	waddstr(w, prompt);

	echo();
	curs_set(1);

	int key;
	while ((key = wgetch(w)) == ERR)
		;

#ifdef HAVE_GETMOUSE
	/* ignore mouse events */
	if (key == KEY_MOUSE)
		return screen_getch(prompt);
#endif

	noecho();
	curs_set(0);

	return key;
}

bool
screen_get_yesno(const char *prompt, bool def)
{
	/* NOTE: if one day a translator decides to use a multi-byte character
	   for one of the yes/no keys, we'll have to parse it properly */

	int key = tolower(screen_getch(prompt));
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
	struct window *window = &screen.status_bar.window;
	WINDOW *w = window->w;
	char *line = NULL;

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
	struct window *window = &screen.status_bar.window;
	WINDOW *w = window->w;

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);

	if (prompt == NULL)
		prompt = _("Password");
	char *ret = wreadln_masked(w, prompt, NULL, window->cols, NULL, NULL);

	curs_set(0);
	return ret;
}

void
screen_display_completion_list(GList *list)
{
	static GList *prev_list = NULL;
	static guint prev_length = 0;
	static guint offset = 0;
	WINDOW *w = screen.main_window.w;

	unsigned length = g_list_length(list);
	if (list == prev_list && length == prev_length) {
		offset += screen.main_window.rows;
		if (offset >= length)
			offset = 0;
	} else {
		prev_list = list;
		prev_length = length;
		offset = 0;
	}

	colors_use(w, COLOR_STATUS_ALERT);

	unsigned y = 0;
	while (y < screen.main_window.rows) {
		GList *item = g_list_nth(list, y+offset);

		wmove(w, y++, 0);
		wclrtoeol(w);
		if (item) {
			gchar *tmp = g_strdup(item->data);
			waddstr(w, g_basename(tmp));
			g_free(tmp);
		}
	}

	wrefresh(w);
	doupdate();
	colors_use(w, COLOR_LIST);
}

#ifndef NCMPC_MINI
void
set_xterm_title(const char *format, ...)
{
	/* the current xterm title exists under the WM_NAME property */
	/* and can be retrieved with xprop -id $WINDOWID */

	if (options.enable_xterm_title) {
		if (g_getenv("WINDOWID")) {
			va_list ap;
			va_start(ap,format);
			char *msg = g_strdup_vprintf(format,ap);
			va_end(ap);
			printf("\033]0;%s\033\\", msg);
			fflush(stdout);
			g_free(msg);
		} else
			options.enable_xterm_title = FALSE;
	}
}
#endif
