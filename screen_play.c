#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "options.h"
#include "support.h"
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

  if( index==c->song_id && !IS_STOPPED(c->status->state) )
    {
      *highlight = 1;
    }

  return mpc_get_song_name(song);
}

static int
center_playing_item(screen_t *screen, mpd_client_t *c)
{
  list_window_t *lw = screen->playlist;
  int length = c->playlist_length;
  int offset = lw->selected-lw->start;
  
  if( !lw || length<lw->rows || IS_STOPPED(c->status->state) )
    return 0;

  /* try to center the song that are playing */
  lw->start = c->song_id-(lw->rows/2);
  if( lw->start+lw->rows > length )
    lw->start = length-lw->rows;
  if( lw->start<0 )
    lw->start=0;

  /* make sure the cursor is in the window */
  lw->selected = lw->start+offset;
  list_window_check_selected(lw, length);

  lw->clear = 1;
  lw->repaint = 1;

  return 0;
}

static int
handle_save_playlist(screen_t *screen, mpd_client_t *c)
{
  char *filename, *filename_utf8;

  filename=screen_getstr(screen->status_window.w, "Save playlist as: ");
  filename=trim(filename);
  if( filename==NULL || filename[0]=='\0' )
    return -1;
  /* convert filename to utf-8 */
  filename_utf8 = locale_to_utf8(filename);
  /* send save command to mpd */
  mpd_sendSaveCommand(c->connection, filename_utf8);
  mpd_finishCommand(c->connection);
  g_free(filename_utf8);
  /* handle errors */
  if( mpc_error(c))
    {
      if(  mpc_error_str(c) )
	{
	  char *str = utf8_to_locale(mpc_error_str(c));
	  screen_status_message(str);
	  g_free(str);
	}
      else
	screen_status_printf("Error: Unable to save playlist as %s", filename);
      mpd_clearError(c->connection);
      beep();
      return -1;
    }
  /* success */
  screen_status_printf("Saved %s", filename);
  g_free(filename);
  /* update the file list if it has been initalized */
  if( c->filelist )
    {
      mpc_update_filelist(c);
      list_window_check_selected(screen->filelist, c->filelist_length);
    }
  return 0;
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
  wnoutrefresh(screen->playlist->w);
}

void
play_update(screen_t *screen, mpd_client_t *c)
{
  if( options.auto_center )
    {
      static int prev_song_id = 0;
      
      if( prev_song_id != c->song_id )	
	{
	  center_playing_item(screen, c);
	  prev_song_id = c->song_id;
	}
    }

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
      wnoutrefresh(screen->playlist->w);
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
      if( song )
	{
	  file_clear_highlight(c, song);
	  mpd_sendDeleteCommand(c->connection, screen->playlist->selected);
	  mpd_finishCommand(c->connection);
	  if( !mpc_error(c) )
	    {
	      screen_status_printf("Removed \'%s\' from playlist!",
				   mpc_get_song_name(song));

	    }
	}
      return 1;
    case CMD_SAVE_PLAYLIST:
      handle_save_playlist(screen, c);
      return 1;
    case CMD_SCREEN_UPDATE:
      center_playing_item(screen, c);
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
