#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
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
  { 0, CMD_STOP,           NULL },
  { 0, CMD_PAUSE,          NULL },
  { 0, CMD_TRACK_NEXT,     NULL },
  { 0, CMD_TRACK_PREVIOUS, NULL },
  { 0, CMD_VOLUME_DOWN,    NULL },
  { 0, CMD_VOLUME_UP,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_LIST_PREVIOUS,  NULL },
  { 0, CMD_LIST_NEXT,      NULL },
  { 0, CMD_LIST_PREVIOUS_PAGE, NULL }, 
  { 0, CMD_LIST_NEXT_PAGE, NULL },
  { 0, CMD_LIST_FIRST,     NULL },
  { 0, CMD_LIST_LAST,      NULL },

  { 0, CMD_LIST_FIND,      NULL },
  { 0, CMD_LIST_RFIND,     NULL },
  { 0, CMD_LIST_FIND_NEXT, NULL },
  { 0, CMD_LIST_RFIND_NEXT,  NULL },
  { 0, CMD_TOGGLE_FIND_WRAP, NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_NEXT,    NULL },
  { 0, CMD_SCREEN_HELP,    NULL },
  { 0, CMD_SCREEN_PLAY,    NULL },
  { 0, CMD_SCREEN_FILE,    NULL },
  { 0, CMD_QUIT,           NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE, "    Keys - Playlist screen " },
  { 0, CMD_NONE, "  --------------------------" },
  { 0, CMD_PLAY,           "Play" },
  { 0, CMD_DELETE,         NULL },
  { 0, CMD_SHUFFLE,        NULL },
  { 0, CMD_CLEAR,          NULL },
  { 0, CMD_SAVE_PLAYLIST,  NULL },
  { 0, CMD_REPEAT,         NULL },
  { 0, CMD_RANDOM,         NULL },
  { 0, CMD_SCREEN_UPDATE,  "Center" },
  { 0, CMD_TOGGLE_AUTOCENTER, NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE, "    Keys - Browse screen " },
  { 0, CMD_NONE, "  ------------------------" },
  { 0, CMD_PLAY,            "Enter directory" },
  { 0, CMD_SELECT,          NULL },
  { 0, CMD_DELETE,          NULL },
  { 0, CMD_SCREEN_UPDATE,   NULL },
  { 0, CMD_NONE, NULL },
  { 0, CMD_NONE, NULL },
  { 1, CMD_NONE, " " PACKAGE " version " VERSION },
  {-1, CMD_NONE, NULL }
};

static int help_text_rows = -1;



static char *
list_callback(int index, int *highlight, void *data)
{
  static char buf[256];

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
	    return help_text[index].text;
	  else
	    return "  ";
	}
      if( help_text[index].text )
	snprintf(buf, 256, 
		 "%20s : %s", 
		 get_key_names(help_text[index].command, TRUE),
		 help_text[index].text);
      else
	snprintf(buf, 256, 
		 "%20s : %s", 
		 get_key_names(help_text[index].command, TRUE),
		 get_key_description(help_text[index].command));
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
  int retval;

  retval = list_window_cmd(screen->helplist, help_text_rows, cmd);
  if( !retval )
    return screen_find(screen, c, 
		       screen->helplist, help_text_rows,
		       cmd, list_callback);

  return retval;
}
