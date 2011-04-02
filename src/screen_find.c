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

#include "screen_find.h"
#include "screen_utils.h"
#include "screen_message.h"
#include "screen.h"
#include "i18n.h"
#include "options.h"
#include "ncmpc.h"

#define FIND_PROMPT  _("Find")
#define RFIND_PROMPT _("Find backward")
#define JUMP_PROMPT _("Jump")

/* query user for a string and find it in a list window */
int
screen_find(struct list_window *lw, command_t findcmd,
	    list_window_callback_fn_t callback_fn,
	    void *callback_data)
{
	int reversed = 0;
	bool found;
	const char *prompt = FIND_PROMPT;
	char *value = options.find_show_last_pattern ? (char *) -1 : NULL;

	if (findcmd == CMD_LIST_RFIND || findcmd == CMD_LIST_RFIND_NEXT) {
		prompt = RFIND_PROMPT;
		reversed = 1;
	}

	switch (findcmd) {
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
		if (screen.findbuf) {
			g_free(screen.findbuf);
			screen.findbuf=NULL;
		}
		/* continue... */

	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		if (!screen.findbuf)
			screen.findbuf=screen_readln(prompt,
						     value,
						     &screen.find_history,
						     NULL);

		if (screen.findbuf == NULL)
			return 1;

		found = reversed
			? list_window_rfind(lw,
					    callback_fn, callback_data,
					    screen.findbuf,
					    options.find_wrap,
					    options.bell_on_wrap)
			: list_window_find(lw,
					   callback_fn, callback_data,
					   screen.findbuf,
					   options.find_wrap,
					   options.bell_on_wrap);
		if (!found) {
			screen_status_printf(_("Unable to find \'%s\'"),
					     screen.findbuf);
			screen_bell();
		}
		return 1;
	default:
		break;
	}
	return 0;
}

/* query user for a string and jump to the entry
 * which begins with this string while the users types */
void
screen_jump(struct list_window *lw,
		list_window_callback_fn_t callback_fn,
	    list_window_paint_callback_t paint_callback,
		void *callback_data)
{
	char *search_str, *iter, *temp;
	const int WRLN_MAX_LINE_SIZE = 1024;
	int key = 65;
	command_t cmd;

	if (screen.findbuf) {
		g_free(screen.findbuf);
		screen.findbuf = NULL;
	}
	screen.findbuf = g_malloc0(WRLN_MAX_LINE_SIZE);
	/* In screen.findbuf is the whole string which is displayed in the status_window
	 * and search_str is the string the user entered (without the prompt) */
	search_str = screen.findbuf + g_snprintf(screen.findbuf, WRLN_MAX_LINE_SIZE, "%s: ", JUMP_PROMPT);
	iter = search_str;

	while(1) {
		key = screen_getch(screen.findbuf);
		/* if backspace or delete was pressed, process instead of ending loop */
		if (key == KEY_BACKSPACE || key == KEY_DC) {
			int i;
			if (search_str <= g_utf8_find_prev_char(screen.findbuf, iter))
				iter = g_utf8_find_prev_char(screen.findbuf, iter);
			for (i = 0; *(iter + i) != '\0'; i++)
				*(iter + i) = '\0';
			continue;
		}
		/* if a control key was pressed, end loop */
		else if (g_ascii_iscntrl(key) || key == KEY_NPAGE || key == KEY_PPAGE) {
			break;
		}
		else {
			*iter = key;
			if (iter < screen.findbuf + WRLN_MAX_LINE_SIZE - 3)
				++iter;
		}
		list_window_jump(lw, callback_fn, callback_data, search_str);

		/* repaint the list_window */
		if (paint_callback != NULL)
			list_window_paint2(lw, paint_callback, callback_data);
		else
			list_window_paint(lw, callback_fn, callback_data);
		wrefresh(lw->w);
	}

	/* ncmpc should get the command */
	ungetch(key);
	if ((cmd=get_keyboard_command()) != CMD_NONE)
		do_input_event(cmd);

	temp = g_strdup(search_str);
	g_free(screen.findbuf);
	screen.findbuf = temp;
}
