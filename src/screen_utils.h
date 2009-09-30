/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef SCREEN_UTILS_H
#define SCREEN_UTILS_H

#include "config.h"
#include "list_window.h"
#include "command.h"

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

struct mpdclient;

/* sound an audible and/or visible bell */
void screen_bell(void);

/* read a character from the status window */
int screen_getch(WINDOW *w, const char *prompt);

char *
screen_read_password(WINDOW *w, const char *prompt);

char *screen_readln(WINDOW *w, const char *prompt, const char *value,
		    GList **history, GCompletion *gcmp);

/* query user for a string and find it in a list window */
int screen_find(struct list_window *lw,
		int rows,
		command_t findcmd,
		list_window_callback_fn_t callback_fn,
		void *callback_data);

/* query user for a string and jump to the entry
 * which begins with this string while the users types */
void screen_jump(struct list_window *lw,
		list_window_callback_fn_t callback_fn,
		void *callback_data);

void screen_display_completion_list(GList *list);

#ifndef NCMPC_MINI
void set_xterm_title(const char *format, ...);
#endif

#endif
