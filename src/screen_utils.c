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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "ncmpc.h"
#include "mpdclient.h"
#include "support.h"
#include "command.h"
#include "options.h"
#include "list_window.h"
#include "colors.h"
#include "wreadln.h"
#include "screen.h"

#define FIND_PROMPT  _("Find: ")
#define RFIND_PROMPT _("Find backward: ")

int
screen_getch(WINDOW *w, char *prompt)
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
  timeout(-1);

  while( (key=wgetch(w)) == ERR )
    ;
  
  if( key==KEY_RESIZE )
    screen_resize();

  noecho();
  curs_set(0);
  timeout(SCREEN_TIMEOUT);

  return key;
}

char *
screen_readln(WINDOW *w, 
	      char *prompt, 
	      char *value,
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
screen_getstr(WINDOW *w, char *prompt)
{
  return screen_readln(w, prompt, NULL, NULL, NULL);
}


/* query user for a string and find it in a list window */
int 
screen_find(screen_t *screen,
	    mpdclient_t *c,
	    list_window_t *lw, 
	    int rows,
	    command_t findcmd,
	    list_window_callback_fn_t callback_fn)
{
  int reversed = 0;
  int retval   = 0;
  char *prompt = FIND_PROMPT;

  if( findcmd==CMD_LIST_RFIND ||findcmd==CMD_LIST_RFIND_NEXT ) 
    {
      prompt = RFIND_PROMPT;
      reversed = 1;
    }

  switch(findcmd)
    {
    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
      if( screen->findbuf )
	{
	  g_free(screen->findbuf);
	  screen->findbuf=NULL;
	}
      /* continue... */
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      if( !screen->findbuf )
	screen->findbuf=screen_readln(screen->status_window.w,
				      prompt,
				      (char *) -1, //NULL,
				      &screen->find_history,
				      NULL);
      if( !screen->findbuf || !screen->findbuf[0] )
	return 1; 
      if( reversed )
	retval = list_window_rfind(lw, 
				   callback_fn,
				   c, 
				   screen->findbuf,
				   options.find_wrap,
				   rows);
      else
	retval = list_window_find(lw,
				  callback_fn,
				  c,
				  screen->findbuf,
				  options.find_wrap);
      if( retval == 0 )
	{
	  lw->repaint  = 1;
	}
      else
	{
	  screen_status_printf(_("Unable to find \'%s\'"), screen->findbuf);
	  beep();
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
  WINDOW *w = screen->main_window.w;
  gint y=0;

  colors_use(w, COLOR_STATUS_ALERT);
  while( y<screen->main_window.rows )
    {
      wmove(w, y++, 0);
      wclrtoeol(w);
      if( list )
	{
	  gchar *tmp = g_strdup(list->data);
	  waddstr(w, basename(tmp));
	  g_free(tmp);
	  list = list->next;
	}
    }
  wrefresh(w);
  doupdate();
  colors_use(w, COLOR_LIST);
}
