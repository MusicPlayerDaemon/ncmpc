#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_file.h"
#include "screen_play.h"


#define BUFSIZE 256

static char *
list_callback(int index, int *highlight, void *data)
{
  mpd_client_t *c = (mpd_client_t *) data;
  mpd_Song *song;

  *highlight = 0;
  if( (song=mpc_playlist_get_song(c, index)) == NULL )
    {
      return NULL;
    }

  if( IS_PLAYING(c->status->state) && index==c->song_id )
    {
      *highlight = 1;
    }

  return mpc_get_song_name(song);
}

void 
play_open(screen_t *screen, mpd_client_t *c)
{

}

void 
play_close(screen_t *screen, mpd_client_t *c)
{
}

void
play_paint(screen_t *screen, mpd_client_t *c)
{
  list_window_t *w = screen->playlist;
 
  w->clear = 1;
  
  list_window_paint(screen->playlist, list_callback, (void *) c);
  wrefresh(screen->playlist->w);
}

void
play_update(screen_t *screen, mpd_client_t *c)
{
  if( c->playlist_updated )
    {
      if( screen->playlist->selected >= c->playlist_length )
	screen->playlist->selected = c->playlist_length-1;
      if( screen->playlist->start    >= c->playlist_length )
	list_window_reset(screen->playlist);

      play_paint(screen, c);
      c->playlist_updated = 0;
    }
  else if( screen->playlist->repaint || 1)
    {
      list_window_paint(screen->playlist, list_callback, (void *) c);
      wrefresh(screen->playlist->w);
      screen->playlist->repaint = 0;
    }
}

int
play_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
  mpd_Song *song;

  switch(cmd)
    {
    case CMD_DELETE:
      song = mpc_playlist_get_song(c, screen->playlist->selected);
      file_clear_highlight(c, song);
      mpd_sendDeleteCommand(c->connection, screen->playlist->selected);
      mpd_finishCommand(c->connection);
      screen_status_printf("Removed \'%s\' from playlist!",
			   mpc_get_song_name(song));
      return 1;
    case CMD_SAVE_PLAYLIST:
      screen_status_printf("Sorry, playlist saving not implemented yet!");
      return 1;
    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      return screen_find(screen, c, 
			 screen->playlist, c->playlist_length,
			 cmd, list_callback);
    default:
      break;
    }
  return list_window_cmd(screen->playlist, c->playlist_length, cmd) ;
}
