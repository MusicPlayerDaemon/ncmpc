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
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "ncmpc.h"
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

static list_window_t *lw = NULL;

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

  filename=screen_getstr(screen->status_window.w, _("Save playlist as: "));
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
	screen_status_printf(_("Error: Unable to save playlist as %s"), 
			     filename);
      mpd_clearError(c->connection);
      beep();
      return -1;
    }
  /* success */
  screen_status_printf(_("Saved %s"), filename);
  g_free(filename);
  /* update the file list if it has been initalized */
  if( c->filelist )
    {
      list_window_t *file_lw = get_filelist_window();

      mpc_update_filelist(c);
      list_window_check_selected(file_lw, c->filelist_length);
    }
  return 0;
}

static void
play_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
}

static void
play_resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}


static void
play_exit(void)
{
  list_window_free(lw);
}

static char *
play_title(void)
{
  return _("Music Player Client - Playlist");
}

static void
play_paint(screen_t *screen, mpd_client_t *c)
{ 
  lw->clear = 1;

  list_window_paint(lw, list_callback, (void *) c);
  wnoutrefresh(lw->w);
}

static void
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
      if( lw->selected >= c->playlist_length )
	lw->selected = c->playlist_length-1;
      if( lw->start    >= c->playlist_length )
	list_window_reset(lw);

      play_paint(screen, c);
      c->playlist_updated = 0;
    }
  else if( lw->repaint || 1)
    {
      list_window_paint(lw, list_callback, (void *) c);
      wnoutrefresh(lw->w);
      lw->repaint = 0;
    }
}

static int
play_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
  switch(cmd)
    {
    case CMD_DELETE:
      playlist_delete_song(c, lw->selected);
      return 1;
    case CMD_SAVE_PLAYLIST:
      handle_save_playlist(screen, c);
      return 1;
    case CMD_SCREEN_UPDATE:
      center_playing_item(screen, c);
      return 1;
    case CMD_LIST_MOVE_UP:
      playlist_move_song(c, lw->selected, lw->selected-1);
      break;
    case CMD_LIST_MOVE_DOWN:
      playlist_move_song(c, lw->selected, lw->selected+1);
      break;
    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      return screen_find(screen, c, 
			 lw, c->playlist_length,
			 cmd, list_callback);
    default:
      break;
    }
  return list_window_cmd(lw, c->playlist_length, cmd) ;
}



static list_window_t *
play_lw(void)
{
  return lw;
}

int 
play_get_selected(void)
{
  return lw->selected;
}

int
playlist_move_song(mpd_client_t *c, int old_index, int new_index)
{
  int index1, index2;
  GList *item1, *item2;
  gpointer data1, data2;

  if( old_index==new_index || new_index<0 || new_index>=c->playlist_length )
    return -1;

  /* send the move command to mpd */
  mpd_sendMoveCommand(c->connection, old_index, new_index);
  mpd_finishCommand(c->connection);
  if( mpc_error(c) )
    return -1;

  index1 = MIN(old_index, new_index);
  index2 = MAX(old_index, new_index);
  item1 = g_list_nth(c->playlist, index1);
  item2 = g_list_nth(c->playlist, index2);
  data1 = item1->data;
  data2 = item2->data;

  /* move the second item */
  D(fprintf(stderr, "move second item [%d->%d]...\n", index2, index1));
  c->playlist = g_list_remove(c->playlist, data2);
  c->playlist = g_list_insert_before(c->playlist, item1, data2);

  /* move the first item */
  if( index2-index1 >1 )
    {
      D(fprintf(stderr, "move first item [%d->%d]...\n", index1, index2));
      item2 = g_list_nth(c->playlist, index2);
      c->playlist = g_list_remove(c->playlist, data1);
      c->playlist = g_list_insert_before(c->playlist, item2, data1);
    }
  
  /* increment the playlist id, so we dont retrives a new playlist */
  c->playlist_id++;

  /* make shure the playlist is repainted */
  lw->clear = 1;
  lw->repaint = 1;

  /* keep song selected */
  lw->selected = new_index;

  return 0;
}

int
playlist_add_song(mpd_client_t *c, mpd_Song *song)
{
  if( !song || !song->file )
    return -1;

  /* send the add command to mpd */
  mpd_sendAddCommand(c->connection, song->file);
  mpd_finishCommand(c->connection);
  if( mpc_error(c) )
    return -1;

  /* add the song to playlist */
  c->playlist = g_list_append(c->playlist, (gpointer) mpd_songDup(song));
  c->playlist_length++;

  /* increment the playlist id, so we dont retrives a new playlist */
  c->playlist_id++;

  /* make shure the playlist is repainted */
  lw->clear = 1;
  lw->repaint = 1;

  /* set selected highlight in the browse screen */
  file_set_highlight(c, song, 1);

  return 0;
}

int
playlist_delete_song(mpd_client_t *c, int index)
{
  mpd_Song *song = mpc_playlist_get_song(c, index);

  if( !song )
    return -1;

  /* send the delete command to mpd */
  mpd_sendDeleteCommand(c->connection, index);
  mpd_finishCommand(c->connection); 
  if( mpc_error(c) )
    return -1;

  /* print a status message */
  screen_status_printf(_("Removed \'%s\' from playlist!"),
		       mpc_get_song_name(song));
  /* clear selected highlight in the browse screen */
  file_set_highlight(c, song, 0);

  /* increment the playlist id, so we dont retrives a new playlist */
  c->playlist_id++;

  /* remove references to the song */
  if( c->song == song )
    {
      c->song = NULL;
      c->song_id = -1;
    }
  
  /* remove the song from the playlist */
  c->playlist = g_list_remove(c->playlist, (gpointer) song);
  c->playlist_length = g_list_length(c->playlist);
  mpd_freeSong(song);

  /* make shure the playlist is repainted */
  lw->clear = 1;
  lw->repaint = 1;
  list_window_check_selected(lw, c->playlist_length);

  return 0;
}


screen_functions_t *
get_screen_playlist(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = play_init;
  functions.exit   = play_exit;
  functions.open   = NULL;
  functions.close  = NULL;
  functions.resize = play_resize;
  functions.paint  = play_paint;
  functions.update = play_update;
  functions.cmd    = play_cmd;
  functions.get_lw = play_lw;
  functions.get_title = play_title;

  return &functions;
}
