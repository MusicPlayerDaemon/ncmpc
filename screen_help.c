/* 
 * $Id: screen_help.c,v 1.8 2004/03/17 13:40:25 kalle Exp $ 
 *
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_help.h"

typedef struct
{
  char highlight;
  command_t command;
  char *text;
} help_text_row_t;

static help_text_row_t help_text[] = 
{
  { 1, CMD_NONE,  "          Keys   " },
  { 0, CMD_NONE,  "        --------" },
  { 0, CMD_STOP,           "Stop" },
  { 0, CMD_PAUSE,          "Pause" },
  { 0, CMD_TRACK_NEXT,     "Next track" },
  { 0, CMD_TRACK_PREVIOUS, "Prevoius track (back)" },
  { 0, CMD_VOLUME_DOWN,    "Volume down" },
  { 0, CMD_VOLUME_UP,      "Volume up" },
  { 0, CMD_NONE, " " },
  { 0, CMD_LIST_NEXT,     "Move cursor up" },
  { 0, CMD_LIST_PREVIOUS, "Move cursor down" },
  { 0, CMD_SCREEN_NEXT,   "Change screen" },
  { 0, CMD_SCREEN_HELP,   "Help screen" },
  { 0, CMD_SCREEN_PLAY,   "Playlist screen" },
  { 0, CMD_SCREEN_FILE,   "Browse screen" },
  { 0, CMD_QUIT,          "Quit" },
  { 0, CMD_NONE, " " },
  { 0, CMD_NONE, " " },
  { 1, CMD_NONE, "    Keys - Playlist screen " },
  { 0, CMD_NONE, "  --------------------------" },
  { 0, CMD_PLAY,    "Play selected entry" },
  { 0, CMD_DELETE,  "Delete selected entry from platlist" },
  { 0, CMD_SHUFFLE, "Shuffle playlist" },
  { 0, CMD_CLEAR,   "Clear playlist" },
  { 0, CMD_REPEAT,  "Toggle repeat mode" },
  { 0, CMD_RANDOM,  "Toggle random mode" },
  { 0, CMD_NONE, " " },
  { 0, CMD_NONE, " " },
  { 1, CMD_NONE, "    Keys - Browse screen " },
  { 0, CMD_NONE, "  ------------------------" },
  { 0, CMD_PLAY,   "Change to selected directory" },
  { 0, CMD_SELECT, "Add/Remove selected file" },
  { 0, CMD_NONE, " " },
  { 0, CMD_NONE, " " },
  { 1, CMD_NONE, " " PACKAGE " version " VERSION },
  { 0, CMD_NONE, NULL }
};

static int help_text_rows = -1;



static char *
list_callback(int index, int *highlight, void *data)
{
  static char buf[256];

  if( help_text_rows<0 )
    {
      help_text_rows = 0;
      while( help_text[help_text_rows].text )
	help_text_rows++;
    }

  *highlight = 0;
  if( index<help_text_rows )
    {
      *highlight = help_text[index].highlight;
      if( help_text[index].command == CMD_NONE )
	return help_text[index].text;
      snprintf(buf, 256, 
	       "%20s : %s", 
	       command_get_keys(help_text[index].command),
	       help_text[index].text);
      return buf;
    }

  return NULL;
}


void 
help_open(screen_t *screen, mpd_client_t *c)
{
}

void 
help_close(screen_t *screen, mpd_client_t *c)
{
}

void 
help_paint(screen_t *screen, mpd_client_t *c)
{
  list_window_t *w = screen->helplist;

  w->clear = 1;
  list_window_paint(screen->helplist, list_callback, NULL);
  wrefresh(screen->helplist->w);
}

void 
help_update(screen_t *screen, mpd_client_t *c)
{  
  list_window_t *w = screen->helplist;
 
  if( w->repaint )
    {
      list_window_paint(screen->helplist, list_callback, NULL);
      wrefresh(screen->helplist->w);
      w->repaint = 0;
    }
}


int 
help_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
 switch(cmd)
    {
    case CMD_LIST_PREVIOUS:
      list_window_previous(screen->helplist);
      screen->helplist->repaint=1;
      break;
    case CMD_LIST_NEXT:
      list_window_next(screen->helplist, help_text_rows);
      screen->helplist->repaint=1;
      break;
    case CMD_LIST_FIRST:
      list_window_first(screen->helplist);
      screen->helplist->repaint  = 1;
      break;
    case CMD_LIST_LAST:
      list_window_last(screen->helplist, help_text_rows);
      screen->helplist->repaint  = 1;
      break;
    case CMD_LIST_PREVIOUS_PAGE:
      list_window_previous_page(screen->helplist);
      screen->helplist->repaint  = 1;
      break;
    case CMD_LIST_NEXT_PAGE:
      list_window_next_page(screen->helplist, help_text_rows);
      screen->helplist->repaint  = 1;
      break;
    case CMD_LIST_FIND:
      if( screen->findbuf )
	{
	  free(screen->findbuf);
	  screen->findbuf=NULL;
	}
      /* continue... */
    case CMD_LIST_FIND_NEXT:
      if( !screen->findbuf )
	screen->findbuf=screen_readln(screen->status_window.w, "/");
      if( list_window_find(screen->helplist,
			   list_callback,
			   c,
			   screen->findbuf) == 0 )
	{
	  screen->helplist->repaint  = 1;
	}
      else
	{
	  screen_status_printf("Unable to find \'%s\'", screen->findbuf);
	  beep();
	}
      break;
    default:
      return 0;
    }
  return 1;
}
