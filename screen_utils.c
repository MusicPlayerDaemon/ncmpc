/* 
 * (c) 2004 by Kalle Wallin (kaw@linux.se)
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
#include "libmpdclient.h"
#include "mpc.h"
#include "support.h"
#include "command.h"
#include "options.h"
#include "list_window.h"
#include "screen.h"

#define FIND_PROMPT  "Find: "
#define RFIND_PROMPT "Find backward: "

int
screen_getch(WINDOW *w, char *prompt)
{
  int key = -1;
  int prompt_len = strlen(prompt);

  wclear(w);  
  wmove(w, 0, 0);
  waddstr(w, prompt);
  wmove(w, 0, prompt_len);
  
  echo();
  curs_set(1);
  timeout(-1);

  key = wgetch(w);
  if( key==KEY_RESIZE )
    screen_resize();

  noecho();
  curs_set(0);
  timeout(SCREEN_TIMEOUT);

  return key;
}


char *
screen_getstr(WINDOW *w, char *prompt)
{
  char buf[256], *line = NULL;
  int prompt_len = strlen(prompt);

  wclear(w);  
  wmove(w, 0, 0);
  waddstr(w, prompt);
  wmove(w, 0, prompt_len);
  
  echo();
  curs_set(1);

  if( wgetnstr(w, buf, 256) == OK )
    line = g_strdup(buf);

  noecho();
  curs_set(0);

  return line;
}


/* query user for a string and find it in a list window */
int 
screen_find(screen_t *screen,
	    mpd_client_t *c,
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
	screen->findbuf=screen_getstr(screen->status_window.w, prompt);
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
	  screen_status_printf("Unable to find \'%s\'", screen->findbuf);
	  beep();
	}
      return 1;
    default:
      break;
    }
  return 0;
}


int
my_waddstr(WINDOW *w, const char *text, int color)
{
  int ret;

  if( options.enable_colors )
    wattron(w, color);
  ret = waddstr(w, text);
  if( options.enable_colors )
    wattroff(w, color);

  return ret;
}

int
my_mvwaddstr(WINDOW *w, int x, int y, const char *text, int color)
{
  int ret;

  if( options.enable_colors )
    wattron(w, color);
  ret = mvwaddstr(w, x, y, text);
  if( options.enable_colors )
    wattroff(w, color);

  return ret;
}

