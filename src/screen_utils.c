/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "screen_utils.h"
#include "screen.h"
#include "mpdclient.h"
#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "options.h"
#include "colors.h"
#include "wreadln.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FIND_PROMPT  _("Find: ")
#define RFIND_PROMPT _("Find backward: ")

void
screen_bell(void)
{
	if (options.audible_bell)
		beep();
	if (options.visible_bell)
		flash();
}

int
screen_getch(WINDOW *w, const char *prompt)
{
	int key = -1;
	int prompt_len = strlen(prompt);

	colors_use(w, COLOR_STATUS_ALERT);
	wclear(w);
	wmove(w, 0, 0);
	waddstr(w, prompt);
	wmove(w, 0, prompt_len);

	echo();
	curs_set(1);

	while ((key=my_wgetch(w)) == ERR)
		;

#ifdef HAVE_GETMOUSE
	/* ignore mouse events */
	if (key == KEY_MOUSE)
		return screen_getch(w, prompt);
#endif

	noecho();
	curs_set(0);

	return key;
}

char *
screen_readln(WINDOW *w,
	      const char *prompt,
	      const char *value,
	      GList **history,
	      GCompletion *gcmp)
{
	char *line = NULL;

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);
	line = wreadln(w, prompt, value, COLS, history, gcmp);
	curs_set(0);
	return line;
}

char *
screen_getstr(WINDOW *w, const char *prompt)
{
	return screen_readln(w, prompt, NULL, NULL, NULL);
}

static char *
screen_read_password(WINDOW *w, const char *prompt)
{
	if (w == NULL) {
		int rows, cols;
		getmaxyx(stdscr, rows, cols);
		/* create window for input */
		w = newwin(1,  cols, rows-1, 0);
		leaveok(w, FALSE);
		keypad(w, TRUE);
	}

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);

	if (prompt == NULL)
		return wreadln_masked(w, _("Password: "), NULL, COLS, NULL, NULL);
	else
		return wreadln_masked(w, prompt, NULL, COLS, NULL, NULL);

	curs_set(0);
}

static gint
_screen_auth(struct mpdclient *c, gint recursion)
{
	mpd_clearError(c->connection);
	if (recursion > 2)
		return 1;
	mpd_sendPasswordCommand(c->connection,  screen_read_password(NULL, NULL));
	mpd_finishCommand(c->connection);
	mpdclient_update(c);
	if (c->connection->errorCode == MPD_ACK_ERROR_PASSWORD)
		return  _screen_auth(c, ++recursion);
	return 0;
}

gint
screen_auth(struct mpdclient *c)
{
	gint ret = _screen_auth(c, 0);
	mpdclient_update(c);
	curs_set(0);
	return ret;
}

/* query user for a string and find it in a list window */
int
screen_find(screen_t *screen,
	    list_window_t *lw,
	    int rows,
	    command_t findcmd,
	    list_window_callback_fn_t callback_fn,
	    void *callback_data)
{
	int reversed = 0;
	int retval = 0;
	const char *prompt = FIND_PROMPT;
	char *value = options.find_show_last_pattern ? (char *) -1 : NULL;

	if (findcmd == CMD_LIST_RFIND || findcmd == CMD_LIST_RFIND_NEXT) {
		prompt = RFIND_PROMPT;
		reversed = 1;
	}

	switch (findcmd) {
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
		if (screen->findbuf) {
			g_free(screen->findbuf);
			screen->findbuf=NULL;
		}
		/* continue... */

	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		if (!screen->findbuf)
			screen->findbuf=screen_readln(screen->status_window.w,
						      prompt,
						      value,
						      &screen->find_history,
						      NULL);

		if (!screen->findbuf || !screen->findbuf[0])
			return 1;

		if (reversed)
			retval = list_window_rfind(lw,
						   callback_fn,
						   callback_data,
						   screen->findbuf,
						   options.find_wrap,
						   rows);
		else
			retval = list_window_find(lw,
						  callback_fn,
						  callback_data,
						  screen->findbuf,
						  options.find_wrap);

		if (retval == 0)
			lw->repaint  = 1;
		else {
			screen_status_printf(_("Unable to find \'%s\'"), screen->findbuf);
			screen_bell();
		}
		return 1;
	default:
		break;
	}
	return 0;
}

void
screen_display_completion_list(screen_t *screen, GList *list)
{
	static GList *prev_list = NULL;
	static guint prev_length = 0;
	static guint offset = 0;
	WINDOW *w = screen->main_window.w;
	guint length, y=0;

	length = g_list_length(list);
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
	while (y < screen->main_window.rows) {
		GList *item = g_list_nth(list, y+offset);

		wmove(w, y++, 0);
		wclrtoeol(w);
		if (item) {
			gchar *tmp = g_strdup(item->data);
			waddstr(w, basename(tmp));
			g_free(tmp);
		}
	}

	wrefresh(w);
	doupdate();
	colors_use(w, COLOR_LIST);
}

void
set_xterm_title(const char *format, ...)
{
	/* the current xterm title exists under the WM_NAME property */
	/* and can be retreived with xprop -id $WINDOWID */

	if (options.enable_xterm_title) {
		if (g_getenv("WINDOWID")) {
			char *msg;
			va_list ap;

			va_start(ap,format);
			msg = g_strdup_vprintf(format,ap);
			va_end(ap);
			printf("%c]0;%s%c", '\033', msg, '\007');
			g_free(msg);
		} else
			options.enable_xterm_title = FALSE;
	}
}
