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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "mpdclient.h"
#include "options.h"

#undef  ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_ADD /* broken with song id's */
#define ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_DELETE
#define ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_MOVE
#define ENABLE_SONG_ID
#define ENABLE_PLCHANGES 

#define MPD_ERROR(c) (c==NULL || c->connection==NULL || c->connection->error)

/* from utils.c */
extern GList *string_list_free(GList *string_list);

/* Error callbacks */
static gint
error_cb(mpdclient_t *c, gint error, gchar *msg)
{
  GList *list = c->error_callbacks;
  
  if( list==NULL )
    fprintf(stderr, "error [%d]: %s\n", (error & 0xFF), msg);

  while(list)
    {
      mpdc_error_cb_t cb = list->data;
      if( cb )
	cb(c, error, msg);
      list=list->next;
    }
  mpd_clearError(c->connection);
  return error;
}

#ifdef DEBUG
// Unused ath the moment
/*
#include "strfsong.h"

static gchar *
get_song_name(mpd_Song *song)
{
  static gchar name[256];

  strfsong(name, 256, "[%artist% - ]%title%|%file%", song);
  return name;
}
*/
#endif

/****************************************************************************/
/*** mpdclient functions ****************************************************/
/****************************************************************************/

gint
mpdclient_finish_command(mpdclient_t *c) 
{
  mpd_finishCommand(c->connection);

  if( c->connection->error )
    {
      gchar *msg = locale_to_utf8(c->connection->errorStr);
      gint error = c->connection->error;
      
      if( error == MPD_ERROR_ACK )
	error = error | (c->connection->errorCode << 8);

      error_cb(c, error, msg);
      g_free(msg);
      return error;
    }

  return 0;
}

mpdclient_t *
mpdclient_new(void)
{
  mpdclient_t *c;

  c = g_malloc0(sizeof(mpdclient_t));

  return c;
}

mpdclient_t *
mpdclient_free(mpdclient_t *c)
{
  mpdclient_disconnect(c);
  g_list_free(c->error_callbacks);
  g_list_free(c->playlist_callbacks);
  g_list_free(c->browse_callbacks);
  g_free(c);

  return NULL;
}

gint
mpdclient_disconnect(mpdclient_t *c)
{
  if( c->connection )
    mpd_closeConnection(c->connection);
  c->connection = NULL;

  if( c->status )
    mpd_freeStatus(c->status);
  c->status = NULL;

  if( c->playlist.list )
    mpdclient_playlist_free(&c->playlist);

  if( c->song )
    c->song = NULL;
  
  return 0;
}

gint
mpdclient_connect(mpdclient_t *c, 
		  gchar *host, 
		  gint port, 
		  gfloat timeout,
		  gchar *password)
{
  gint retval = 0;
  
  /* close any open connection */
  if( c->connection )
    mpdclient_disconnect(c);

  /* connect to MPD */
  c->connection = mpd_newConnection(host, port, timeout);
  if( c->connection->error )
    return error_cb(c, c->connection->error, c->connection->errorStr);

  /* send password */
  if( password )
    {
      mpd_sendPasswordCommand(c->connection, password);
      retval = mpdclient_finish_command(c);
    }
  c->need_update = TRUE;

  return retval;
}

gint
mpdclient_update(mpdclient_t *c)
{
  gint retval = 0;

  if( MPD_ERROR(c) )
    return -1;

  /* free the old status */
  if( c->status )
    mpd_freeStatus(c->status);
  
  /* retreive new status */
  mpd_sendStatusCommand(c->connection);
  c->status = mpd_getStatus(c->connection);
  if( (retval=mpdclient_finish_command(c)) )
    return retval;
#ifdef DEBUG
  if( c->status->error )
    D("status> %s\n", c->status->error);
#endif

  /* check if the playlist needs an update */
  if( c->playlist.id != c->status->playlist )
    {
      if( c->playlist.list )
	retval = mpdclient_playlist_update_changes(c);
      else
	retval = mpdclient_playlist_update(c);
    }

  /* update the current song */
  if( !c->song || c->status->songid != c->song->id )
    {
      c->song = playlist_get_song(c, c->status->song);
    }

  c->need_update = FALSE;

  return retval;
}


/****************************************************************************/
/*** MPD Commands  **********************************************************/
/****************************************************************************/

gint 
mpdclient_cmd_play(mpdclient_t *c, gint index)
{
#ifdef ENABLE_SONG_ID
  mpd_Song *song = playlist_get_song(c, index);

  D("Play id:%d\n", song ? song->id : -1);
  if( song )
    mpd_sendPlayIdCommand(c->connection, song->id);
  else
    mpd_sendPlayIdCommand(c->connection, MPD_PLAY_AT_BEGINNING);
#else
  mpd_sendPlayCommand(c->connection, index);
#endif
  c->need_update = TRUE;
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_pause(mpdclient_t *c, gint value)
{
  mpd_sendPauseCommand(c->connection, value);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_stop(mpdclient_t *c)
{
  mpd_sendStopCommand(c->connection);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_next(mpdclient_t *c)
{
  mpd_sendNextCommand(c->connection);
  c->need_update = TRUE;
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_prev(mpdclient_t *c)
{
  mpd_sendPrevCommand(c->connection);
  c->need_update = TRUE;
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_seek(mpdclient_t *c, gint id, gint pos)
{
  D("Seek id:%d\n", id);
  mpd_sendSeekIdCommand(c->connection, id, pos);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_shuffle(mpdclient_t *c)
{
  mpd_sendShuffleCommand(c->connection);
  c->need_update = TRUE;
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_clear(mpdclient_t *c)
{
  gint retval = 0;

  mpd_sendClearCommand(c->connection);
  retval = mpdclient_finish_command(c);
  /* call playlist updated callback */
  mpdclient_playlist_callback(c, PLAYLIST_EVENT_CLEAR, NULL);
  c->need_update = TRUE;
  return retval;
}

gint 
mpdclient_cmd_repeat(mpdclient_t *c, gint value)
{
  mpd_sendRepeatCommand(c->connection, value);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_random(mpdclient_t *c, gint value)
{
  mpd_sendRandomCommand(c->connection, value);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_crossfade(mpdclient_t *c, gint value)
{
  mpd_sendCrossfadeCommand(c->connection, value);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_db_update_utf8(mpdclient_t *c, gchar *path)
{
  mpd_sendUpdateCommand(c->connection, path ? path : "");
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_volume(mpdclient_t *c, gint value)
{
  mpd_sendSetvolCommand(c->connection, value);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_add_path_utf8(mpdclient_t *c, gchar *path_utf8)
{
  mpd_sendAddCommand(c->connection, path_utf8);
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_add_path(mpdclient_t *c, gchar *path)
{
  gint retval;
  gchar *path_utf8 = locale_to_utf8(path);

  retval=mpdclient_cmd_add_path_utf8(c, path_utf8);
  g_free(path_utf8);
  return retval;
}

gint 
mpdclient_cmd_add(mpdclient_t *c, mpd_Song *song)
{ 
  gint retval = 0;

  if( !song || !song->file )
    return -1;

  /* send the add command to mpd */
  mpd_sendAddCommand(c->connection, song->file);
  if( (retval=mpdclient_finish_command(c)) )
    return retval;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_ADD
  /* add the song to playlist */
  c->playlist.list = g_list_append(c->playlist.list, mpd_songDup(song));
  c->playlist.length++;

  /* increment the playlist id, so we dont retrives a new playlist */
  c->playlist.id++;

  /* call playlist updated callback */
  mpdclient_playlist_callback(c, PLAYLIST_EVENT_ADD, (gpointer) song);
#else
  c->need_update = TRUE;
#endif

  return 0;
}

gint
mpdclient_cmd_delete(mpdclient_t *c, gint index)
{
  gint retval = 0;
  mpd_Song *song = playlist_get_song(c, index);

  if( !song )
    return -1;

  /* send the delete command to mpd */
#ifdef ENABLE_SONG_ID
  D("Delete id:%d\n", song->id);
  mpd_sendDeleteIdCommand(c->connection, song->id);
#else
  mpd_sendDeleteCommand(c->connection, index);
#endif
  if( (retval=mpdclient_finish_command(c)) )
    return retval;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_DELETE
  /* increment the playlist id, so we dont retrive a new playlist */
  c->playlist.id++;

  /* remove the song from the playlist */
  c->playlist.list = g_list_remove(c->playlist.list, (gpointer) song);
  c->playlist.length = g_list_length(c->playlist.list);

  /* call playlist updated callback */
  mpdclient_playlist_callback(c, PLAYLIST_EVENT_DELETE, (gpointer) song);

  /* remove references to the song */
  if( c->song == song )
    {
      c->song = NULL;   
      c->need_update = TRUE;
    }

  /* free song */
  mpd_freeSong(song);  

#else
  c->need_update = TRUE;
#endif

  return 0;
}

gint
mpdclient_cmd_move(mpdclient_t *c, gint old_index, gint new_index)
{
  gint n, index1, index2;
  GList *item1, *item2;
  gpointer data1, data2;
  mpd_Song *song1, *song2;

  if( old_index==new_index || new_index<0 || new_index>=c->playlist.length )
    return -1;

  song1 = playlist_get_song(c, old_index);
  song2 = playlist_get_song(c, new_index);

  /* send the move command to mpd */  
#ifdef ENABLE_SONG_ID
  D("Swapping id:%d with id:%d\n", song1->id, song2->id);
  mpd_sendSwapIdCommand(c->connection, song1->id, song2->id);
#else
  D("Moving index %d to id:%d\n", old_index, new_index);
  mpd_sendMoveCommand(c->connection, old_index, new_index);
#endif
  if( (n=mpdclient_finish_command(c)) )
    return n;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_MOVE
  /* update the songs position field */
  n = song1->pos;
  song1->pos = song2->pos;
  song2->pos = n;
  index1 = MIN(old_index, new_index);
  index2 = MAX(old_index, new_index);
  /* retreive the list items */
  item1 = g_list_nth(c->playlist.list, index1);
  item2 = g_list_nth(c->playlist.list, index2);
  /* retrieve the songs */
  data1 = item1->data;
  data2 = item2->data;

  /* move the second item */
  c->playlist.list = g_list_remove(c->playlist.list, data2);
  c->playlist.list = g_list_insert_before(c->playlist.list, item1, data2);

  /* move the first item */
  if( index2-index1 >1 )
    {
      item2 = g_list_nth(c->playlist.list, index2);
      c->playlist.list = g_list_remove(c->playlist.list, data1);
      c->playlist.list = g_list_insert_before(c->playlist.list, item2, data1);
    }

  /* increment the playlist id, so we dont retrives a new playlist */
  c->playlist.id++;

#else
  c->need_update = TRUE;
#endif 

  /* call playlist updated callback */
  D("move> new_index=%d, old_index=%d\n", new_index, old_index);
  mpdclient_playlist_callback(c, PLAYLIST_EVENT_MOVE, (gpointer) &new_index);

  return 0;
}

gint 
mpdclient_cmd_save_playlist_utf8(mpdclient_t *c, gchar *filename_utf8)
{
  gint retval = 0;

  mpd_sendSaveCommand(c->connection, filename_utf8);
  if( (retval=mpdclient_finish_command(c)) == 0 )
    mpdclient_browse_callback(c, BROWSE_PLAYLIST_SAVED, NULL);
  return retval;
}

gint 
mpdclient_cmd_save_playlist(mpdclient_t *c, gchar *filename)
{
  gint retval = 0;
  gchar *filename_utf8 = locale_to_utf8(filename);
  
  retval = mpdclient_cmd_save_playlist_utf8(c, filename);
  g_free(filename_utf8);
  return retval;
}

gint 
mpdclient_cmd_load_playlist_utf8(mpdclient_t *c, gchar *filename_utf8)
{
  mpd_sendLoadCommand(c->connection, filename_utf8);
  c->need_update = TRUE;
  return mpdclient_finish_command(c);
}

gint 
mpdclient_cmd_delete_playlist_utf8(mpdclient_t *c, gchar *filename_utf8)
{
  gint retval = 0;

  mpd_sendRmCommand(c->connection, filename_utf8);
  if( (retval=mpdclient_finish_command(c)) == 0 )
    mpdclient_browse_callback(c, BROWSE_PLAYLIST_DELETED, NULL);
  return retval;
}

gint 
mpdclient_cmd_delete_playlist(mpdclient_t *c, gchar *filename)
{
  gint retval = 0;
  gchar *filename_utf8 = locale_to_utf8(filename);

  retval = mpdclient_cmd_delete_playlist_utf8(c, filename_utf8);
  g_free(filename_utf8);
  return retval;
}


/****************************************************************************/
/*** Callback managment functions *******************************************/
/****************************************************************************/
static void
do_list_callbacks(mpdclient_t *c, GList *list, gint event, gpointer data)
{
  while(list)
    {
      mpdc_list_cb_t fn = list->data;

      fn(c, event, data);
      list=list->next;
    }
}

void
mpdclient_playlist_callback(mpdclient_t *c, int event, gpointer data)
{
  do_list_callbacks(c, c->playlist_callbacks, event, data);
}

void
mpdclient_install_playlist_callback(mpdclient_t *c,mpdc_list_cb_t cb)
{
  c->playlist_callbacks = g_list_append(c->playlist_callbacks, cb);
}

void
mpdclient_remove_playlist_callback(mpdclient_t *c, mpdc_list_cb_t cb)
{
  c->playlist_callbacks = g_list_remove(c->playlist_callbacks, cb);
}

void
mpdclient_browse_callback(mpdclient_t *c, int event, gpointer data)
{
  do_list_callbacks(c, c->browse_callbacks, event, data);
}


void
mpdclient_install_browse_callback(mpdclient_t *c,mpdc_list_cb_t cb)
{
  c->browse_callbacks = g_list_append(c->browse_callbacks, cb);
}

void
mpdclient_remove_browse_callback(mpdclient_t *c, mpdc_list_cb_t cb)
{
  c->browse_callbacks = g_list_remove(c->browse_callbacks, cb);
}

void
mpdclient_install_error_callback(mpdclient_t *c, mpdc_error_cb_t cb)
{
  c->error_callbacks = g_list_append(c->error_callbacks, cb);
}

void
mpdclient_remove_error_callback(mpdclient_t *c, mpdc_error_cb_t cb)
{
  c->error_callbacks = g_list_remove(c->error_callbacks, cb);
}

/****************************************************************************/
/*** Playlist managment functions *******************************************/
/****************************************************************************/

gint
mpdclient_playlist_free(mpdclient_playlist_t *playlist)
{
  GList *list = g_list_first(playlist->list);

  while(list)
    {
      mpd_Song *song = (mpd_Song *) list->data;
      mpd_freeSong(song);
      list=list->next;
    }
  g_list_free(playlist->list);
  memset(playlist, 0, sizeof(mpdclient_playlist_t));
  return 0;
}

/* update playlist */
gint 
mpdclient_playlist_update(mpdclient_t *c)
{
  mpd_InfoEntity *entity;

  D("mpdclient_playlist_update() [%lld]\n", c->status->playlist);

  if( MPD_ERROR(c) )
    return -1;

  if( c->playlist.list )
    mpdclient_playlist_free(&c->playlist);

  mpd_sendPlaylistInfoCommand(c->connection,-1);
  while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
    {
      if(entity->type==MPD_INFO_ENTITY_TYPE_SONG) 
	{
	  mpd_Song *song = mpd_songDup(entity->info.song);

	  c->playlist.list = g_list_append(c->playlist.list, (gpointer) song);
	  c->playlist.length++;
	}
      mpd_freeInfoEntity(entity);
    }
  c->playlist.id = c->status->playlist;
  c->song = NULL;
  c->playlist.updated = TRUE;

  /* call playlist updated callbacks */
  mpdclient_playlist_callback(c, PLAYLIST_EVENT_UPDATED, NULL);

  return mpdclient_finish_command(c);
}

#ifdef ENABLE_PLCHANGES

gint 
mpdclient_compare_songs(gconstpointer a, gconstpointer b)
{
  mpd_Song *song1 = (mpd_Song *) a; 
  mpd_Song *song2 = (mpd_Song *) b; 

  return song1->pos - song2->pos;
}



/* update playlist (plchanges) */
gint 
mpdclient_playlist_update_changes(mpdclient_t *c)
{
  mpd_InfoEntity *entity;

  D("mpdclient_playlist_update_changes() [%lld -> %lld]\n", 
    c->status->playlist, c->playlist.id);

  if( MPD_ERROR(c) )
    return -1;

  mpd_sendPlChangesCommand(c->connection, c->playlist.id); 

  while( (entity=mpd_getNextInfoEntity(c->connection)) != NULL   ) 
    {
      mpd_Song *song = entity->info.song;

      if( song->pos < c->playlist.length )
	{
	  GList *item = g_list_nth(c->playlist.list, song->pos);

	  /* update song */
	  D("updating pos:%d, id=%d [%p] - %s\n", 
	    song->pos, song->id, item, song->file);
	  mpd_freeSong((mpd_Song *) item->data);	      	     
	  item->data = mpd_songDup(song);
	}
      else
	{
	  /* add a new song */
	  D("adding song at pos %d\n", song->pos);
	  c->playlist.list = g_list_append(c->playlist.list, 
					   (gpointer) mpd_songDup(song));
	}
      
    }

  /* remove trailing songs */
  while( c->status->playlistLength < c->playlist.length )
    {
      GList *item = g_list_last(c->playlist.list);

      /* Remove the last playlist entry */
      D("removing song at pos %d\n", ((mpd_Song *) item->data)->pos);
      mpd_freeSong((mpd_Song *) item->data);
      c->playlist.list = g_list_delete_link(c->playlist.list, item);
      c->playlist.length = g_list_length(c->playlist.list);   
    }

  c->song = NULL;
  c->playlist.id = c->status->playlist;
  c->playlist.updated = TRUE;
  c->playlist.length = g_list_length(c->playlist.list);

  mpdclient_playlist_callback(c, PLAYLIST_EVENT_UPDATED, NULL);

  return 0;
}

#else
gint 
mpdclient_playlist_update_changes(mpdclient_t *c)
{
  return mpdclient_playlist_update(c);
}
#endif

mpd_Song *
playlist_get_song(mpdclient_t *c, gint index)
{
  return (mpd_Song *) g_list_nth_data(c->playlist.list, index);
}

GList *
playlist_lookup(mpdclient_t *c, int id)
{
  GList *list = g_list_first(c->playlist.list);

  while( list )
    {
      mpd_Song *song = (mpd_Song *) list->data;
      if( song->id == id )
	return list;
      list=list->next;
    }
  return NULL;
}

mpd_Song *
playlist_lookup_song(mpdclient_t *c, gint id)
{
  GList *list = c->playlist.list;

  while( list )
    {
      mpd_Song *song = (mpd_Song *) list->data;
      if( song->id == id )
	return song;
      list=list->next;
    }
  return NULL;
}

gint 
playlist_get_index(mpdclient_t *c, mpd_Song *song)
{
  return g_list_index(c->playlist.list, song);
}

gint 
playlist_get_index_from_id(mpdclient_t *c, gint id)
{
  return g_list_index(c->playlist.list, playlist_lookup_song(c, id));
}

gint
playlist_get_index_from_file(mpdclient_t *c, gchar *filename)
{
  GList *list = c->playlist.list;
  gint i=0;

  while( list )
    {
      mpd_Song *song = (mpd_Song *) list->data;
      if( strcmp(song->file, filename ) == 0 )	
	return i;
      list=list->next;
      i++;
    }
  return -1;
}


/****************************************************************************/
/*** Filelist functions *****************************************************/
/****************************************************************************/

mpdclient_filelist_t *
mpdclient_filelist_free(mpdclient_filelist_t *filelist)
{
  GList *list = g_list_first(filelist->list);

  D("mpdclient_filelist_free()\n");
  if( list == NULL )
    return NULL;
  while( list!=NULL )
    {
      filelist_entry_t *entry = list->data;

      if( entry->entity )
	mpd_freeInfoEntity(entry->entity);
      g_free(entry);
      list=list->next;
    }
  g_list_free(filelist->list);
  g_free(filelist->path);
  filelist->path = NULL;
  filelist->list = NULL;
  filelist->length = 0;
  g_free(filelist);

  return NULL;
}


mpdclient_filelist_t *
mpdclient_filelist_get(mpdclient_t *c, gchar *path)
{
  mpdclient_filelist_t *filelist;
  mpd_InfoEntity *entity;
  gchar *path_utf8 = locale_to_utf8(path);

  D("mpdclient_filelist_get(%s)\n", path);
  mpd_sendLsInfoCommand(c->connection, path_utf8);
  filelist = g_malloc0(sizeof(mpdclient_filelist_t));
  if( path && path[0] && strcmp(path, "/") )
    {
      /* add a dummy entry for ./.. */
      filelist_entry_t *entry = g_malloc0(sizeof(filelist_entry_t));
      entry->entity = NULL;
      filelist->list = g_list_append(filelist->list, (gpointer) entry);
      filelist->length++;
    }

  while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
    {
      filelist_entry_t *entry = g_malloc0(sizeof(filelist_entry_t));
      
      entry->entity = entity;
      filelist->list = g_list_append(filelist->list, (gpointer) entry);
      filelist->length++;
    }
  
   /* If there's an error, ignore it.  We'll return an empty filelist. */
   mpdclient_finish_command(c);
  
  g_free(path_utf8);
  filelist->path = g_strdup(path);
  filelist->updated = TRUE;

  return filelist;
}

mpdclient_filelist_t *
mpdclient_filelist_search_utf8(mpdclient_t *c, 
			       int exact_match,
			       int table, 
			       gchar *filter_utf8)
{
  mpdclient_filelist_t *filelist;
  mpd_InfoEntity *entity;

  D("mpdclient_filelist_search(%s)\n", filter_utf8);
  if( exact_match )
    mpd_sendFindCommand(c->connection, table, filter_utf8);
  else
    mpd_sendSearchCommand(c->connection, table, filter_utf8);
  filelist = g_malloc0(sizeof(mpdclient_filelist_t));

  while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
    {
      filelist_entry_t *entry = g_malloc0(sizeof(filelist_entry_t));
      
      entry->entity = entity;
      filelist->list = g_list_append(filelist->list, (gpointer) entry);
      filelist->length++;
    }
  
  if( mpdclient_finish_command(c) )
    return mpdclient_filelist_free(filelist);

  filelist->updated = TRUE;

  return filelist;
}


mpdclient_filelist_t *
mpdclient_filelist_search(mpdclient_t *c,
			  int exact_match, 
			  int table, 
			  gchar *filter)
{
  mpdclient_filelist_t *filelist;
  gchar *filter_utf8 = locale_to_utf8(filter);

  D("mpdclient_filelist_search(%s)\n", filter);
  filelist = mpdclient_filelist_search_utf8(c,exact_match,table,filter_utf8);
  g_free(filter_utf8);

  return filelist;
}

mpdclient_filelist_t *
mpdclient_filelist_update(mpdclient_t *c, mpdclient_filelist_t *filelist)
{
  if( filelist != NULL )
    {    
      gchar *path = g_strdup(filelist->path);

      filelist = mpdclient_filelist_free(filelist);
      filelist = mpdclient_filelist_get(c, path);
      g_free(path);
      return filelist;
    }
  return NULL;
}

filelist_entry_t *
mpdclient_filelist_find_song(mpdclient_filelist_t *fl, mpd_Song *song)
{
  GList *list = g_list_first(fl->list);

  while( list && song)
    {
      filelist_entry_t *entry = list->data;
      mpd_InfoEntity *entity  = entry->entity;

      if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song2 = entity->info.song;

	  if( strcmp(song->file, song2->file) == 0 )
	    {
	      return entry;
	    }
	}
      list = list->next;
    }
  return NULL;
}

int
mpdclient_filelist_add_all(mpdclient_t *c, mpdclient_filelist_t *fl)
{
  GList *list = g_list_first(fl->list);

  if( fl->list==NULL || fl->length<1 )
    return 0;

  mpd_sendCommandListBegin(c->connection);
  while( list )
    {
      filelist_entry_t *entry = list->data;
      mpd_InfoEntity *entity  = entry->entity;

      if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entity->info.song;

	  mpd_sendAddCommand(c->connection, song->file);
	}
      list = list->next;
    }
  mpd_sendCommandListEnd(c->connection);
  return mpdclient_finish_command(c);
}








GList *
mpdclient_get_artists_utf8(mpdclient_t *c)
{
  gchar *str = NULL; 
  GList *list = NULL;

  D("mpdclient_get_artists()\n");
  mpd_sendListCommand(c->connection, MPD_TABLE_ARTIST, NULL);
  while( (str=mpd_getNextArtist(c->connection)) )
    {
      list = g_list_append(list, (gpointer) str);
    }
  if( mpdclient_finish_command(c) )
    {
      return string_list_free(list);
    }  

  return list;
}

GList *
mpdclient_get_albums_utf8(mpdclient_t *c, gchar *artist_utf8)
{
  gchar *str = NULL; 
  GList *list = NULL;

  D("mpdclient_get_albums(%s)\n", artist_utf8);
  mpd_sendListCommand(c->connection, MPD_TABLE_ALBUM, artist_utf8);
  while( (str=mpd_getNextAlbum(c->connection)) )
    {
      list = g_list_append(list, (gpointer) str);
    }
  if( mpdclient_finish_command(c) )
    {
      return string_list_free(list);
    }  
  
  return list;
}





