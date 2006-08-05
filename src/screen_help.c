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
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "ncmpc.h"
#include "mpdclient.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"


typedef struct
{
  signed char highlight;
  command_t command;
  char *text;
} help_text_row_t;

static help_text_row_t help_text[] = 
{
  { 1, CMD_NONE,           N_("Keys - Movement") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_LIST_PREVIOUS,  NULL },
  { 0, CMD_LIST_NEXT,      NULL },
  { 0, CMD_LIST_PREVIOUS_PAGE, NULL }, 
  { 0, CMD_LIST_NEXT_PAGE, NULL },
  { 0, CMD_LIST_FIRST,     NULL },
  { 0, CMD_LIST_LAST,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_PREVIOUS,NULL },
  { 0, CMD_SCREEN_NEXT,    NULL },
  { 0, CMD_SCREEN_HELP,    NULL },
  { 0, CMD_SCREEN_PLAY,    NULL },
  { 0, CMD_SCREEN_FILE,    NULL },
#ifdef ENABLE_SEARCH_SCREEN
  { 0, CMD_SCREEN_SEARCH,  NULL },
#endif
#ifdef ENABLE_CLOCK_SCREEN
  { 0, CMD_SCREEN_CLOCK,   NULL },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
  { 0, CMD_SCREEN_KEYDEF,  NULL },
#endif

  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Global") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_STOP,           NULL },
  { 0, CMD_PAUSE,          NULL },
  { 0, CMD_TRACK_NEXT,     NULL },
  { 0, CMD_TRACK_PREVIOUS, NULL },
  { 0, CMD_SEEK_FORWARD,   NULL },
  { 0, CMD_SEEK_BACKWARD,  NULL },
  { 0, CMD_VOLUME_DOWN,    NULL },
  { 0, CMD_VOLUME_UP,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_REPEAT,         NULL },
  { 0, CMD_RANDOM,         NULL },
  { 0, CMD_CROSSFADE,      NULL },
  { 0, CMD_SHUFFLE,        NULL },
  { 0, CMD_DB_UPDATE,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_LIST_FIND,      NULL },
  { 0, CMD_LIST_RFIND,     NULL },
  { 0, CMD_LIST_FIND_NEXT, NULL },
  { 0, CMD_LIST_RFIND_NEXT,  NULL },
  { 0, CMD_TOGGLE_FIND_WRAP, NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_QUIT,           NULL },

  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Playlist screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_PLAY,           N_("Play") },
  { 0, CMD_DELETE,         NULL },
  { 0, CMD_CLEAR,          NULL },
  { 0, CMD_LIST_MOVE_UP,   N_("Move song up") },
  { 0, CMD_LIST_MOVE_DOWN, N_("Move song down") },
  { 0, CMD_ADD,            NULL },
  { 0, CMD_SAVE_PLAYLIST,  NULL },
  { 0, CMD_SCREEN_UPDATE,  N_("Center") },
  { 0, CMD_TOGGLE_AUTOCENTER, NULL },

  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Browse screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_PLAY,           N_("Enter directory/Select and play song") },
  { 0, CMD_SELECT,         NULL },
  { 0, CMD_SAVE_PLAYLIST,  NULL },
  { 0, CMD_DELETE,         N_("Delete playlist") },
  { 0, CMD_SCREEN_UPDATE,  NULL },

#ifdef ENABLE_SEARCH_SCREEN
  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Search screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_SEARCH,  N_("Search") },
  { 0, CMD_PLAY,           N_("Select and play") },
  { 0, CMD_SELECT,         NULL },
  { 0, CMD_SEARCH_MODE,    NULL },
#endif
#ifdef ENABLE_LYRICS_SCREEN
  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Lyrics screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_LYRICS,  N_("View Lyrics") },
  { 0, CMD_SELECT,         N_("(Re)load lyrics") },
  { 0, CMD_INTERRUPT,         N_("Interrupt retrieval") },
#endif
  { 0, CMD_NONE, NULL },
  {-1, CMD_NONE, NULL }
};

static int help_text_rows = -1;
static list_window_t *lw = NULL;


static char *
list_callback(int index, int *highlight, void *data)
{
  static char buf[512];

  if( help_text_rows<0 )
    {
      help_text_rows = 0;
      while( help_text[help_text_rows].highlight != -1 )
	help_text_rows++;
    }

  *highlight = 0;
  if( index<help_text_rows )
    {
      *highlight = help_text[index].highlight > 0;
      if( help_text[index].command == CMD_NONE )
	{
	  if( help_text[index].text )
	    g_snprintf(buf, sizeof(buf), "      %s", _(help_text[index].text));
	  else 
	    if( help_text[index].highlight == 2 )
	      {
		int i;

		for(i=3; i<COLS-3 && i<sizeof(buf); i++)
		  buf[i]='-';
		buf[i] = '\0';
	      }
	    else
	      g_strlcpy(buf, " ", sizeof(buf));
	  return buf;
	}
      if( help_text[index].text )
	g_snprintf(buf, sizeof(buf), 
		   "%20s : %s   ", 
		   get_key_names(help_text[index].command, TRUE),
		   _(help_text[index].text));
      else
	g_snprintf(buf, sizeof(buf), 
		   "%20s : %s   ", 
		   get_key_names(help_text[index].command, TRUE),
		   get_key_description(help_text[index].command));
      return buf;
    }

  return NULL;
}

static void
help_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
  lw->flags = LW_HIDE_CURSOR;
}

static void
help_resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}

static void
help_exit(void)
{
  list_window_free(lw);
}


static char *
help_title(char *str, size_t size)
{
  return _("Help");
}

static void 
help_paint(screen_t *screen, mpdclient_t *c)
{
  lw->clear = 1;
  list_window_paint(lw, list_callback, NULL);
  wrefresh(lw->w);
}

static void 
help_update(screen_t *screen, mpdclient_t *c)
{  
  if( lw->repaint )
    {
      list_window_paint(lw, list_callback, NULL);
      wrefresh(lw->w);
      lw->repaint = 0;
    }
}


static int 
help_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
  lw->repaint=1;
  switch(cmd)
    {
    case CMD_LIST_NEXT:
      if( lw->start+lw->rows < help_text_rows )
	lw->start++;
      return 1;
    case CMD_LIST_PREVIOUS:
      if( lw->start >0 )
	lw->start--;
      return 1;
    case CMD_LIST_FIRST:
      lw->start = 0;
      return 1;
    case CMD_LIST_LAST:
      lw->start = help_text_rows-lw->rows;
      if( lw->start<0 )
	lw->start = 0;
      return 1;
    case CMD_LIST_NEXT_PAGE:
      lw->start = lw->start + lw->rows;
      if( lw->start+lw->rows >= help_text_rows )
	lw->start = help_text_rows-lw->rows;
      if( lw->start<0 )
	lw->start = 0;
       return 1;
    case CMD_LIST_PREVIOUS_PAGE:
      lw->start = lw->start - lw->rows;
      if( lw->start<0 )
	lw->start = 0;
      return 1;
    default:
      break;
    }

  lw->selected = lw->start+lw->rows;
  if( screen_find(screen, c, 
		  lw,  help_text_rows,
		  cmd, list_callback, NULL) )
    {
      /* center the row */
      lw->start = lw->selected-(lw->rows/2);
      if( lw->start+lw->rows > help_text_rows )
	lw->start = help_text_rows-lw->rows;
      if( lw->start<0 )
	lw->start=0;
      return 1;
    }

  return 0;
}

static list_window_t *
help_lw(void)
{
  return lw;
}

screen_functions_t *
get_screen_help(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = help_init;
  functions.exit   = help_exit;
  functions.open   = NULL;
  functions.close  = NULL;
  functions.resize = help_resize;
  functions.paint  = help_paint;
  functions.update = help_update;
  functions.cmd    = help_cmd;
  functions.get_lw = help_lw;
  functions.get_title = help_title;

  return &functions;
}
