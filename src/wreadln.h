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

#ifndef WREADLN_H
#define WREADLN_H

#include "config.h"

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#ifndef NCMPC_MINI

/* completion callback data */
extern void *wrln_completion_callback_data;

/* called after TAB is pressed but before g_completion_complete */
typedef void (*wrln_gcmp_pre_cb_t) (GCompletion *gcmp, gchar *buf, void *data);
extern wrln_gcmp_pre_cb_t wrln_pre_completion_callback;

/* post completion callback */
typedef void (*wrln_gcmp_post_cb_t) (GCompletion *gcmp, gchar *s, GList *l,
				     void *data);
extern wrln_gcmp_post_cb_t wrln_post_completion_callback;

#endif

/* Note, wreadln calls curs_set() and noecho(), to enable cursor and
 * disable echo. wreadln will not restore these settings when exiting! */
gchar *wreadln(WINDOW *w,            /* the curses window to use */
	       const gchar *prompt, /* the prompt string or NULL */
	       const gchar *initial_value, /* initial value or NULL for a empty line
					    * (char *) -1 = get value from history */
	       unsigned x1,              /* the maximum x position or 0 */
	       GList **history,     /* a pointer to a history list or NULL */
	       GCompletion *gcmp    /* a GCompletion structure or NULL */
	       );

gchar *
wreadln_masked(WINDOW *w,
	       const gchar *prompt,
	       const gchar *initial_value,
	       unsigned x1,
	       GList **history,
	       GCompletion *gcmp);

#endif
