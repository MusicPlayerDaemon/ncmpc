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

#include "mpdclient.h"
#include "screen_utils.h"
#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "options.h"
#include "strfsong.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#undef  ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_ADD /* broken with song id's */
#define ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_DELETE
#define ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_MOVE
#define ENABLE_SONG_ID
#define ENABLE_PLCHANGES 

#define BUFSIZE 1024

#define MPD_ERROR(c) (c==NULL || c->connection==NULL || c->connection->error)

/* from utils.c */
extern GList *string_list_free(GList *string_list);


/* filelist sorting functions */
static gint
compare_filelistentry_dir(gconstpointer filelist_entry1,
			  gconstpointer filelist_entry2)
{
	const mpd_InfoEntity *e1, *e2;
	char *key1, *key2;
	int n = 0;

	e1 = ((const filelist_entry_t *)filelist_entry1)->entity;
	e2 = ((const filelist_entry_t *)filelist_entry2)->entity;

	if (e1 && e2 &&
	    e1->type == MPD_INFO_ENTITY_TYPE_DIRECTORY &&
	    e2->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		key1 = g_utf8_collate_key(e1->info.directory->path,-1);
		key2 = g_utf8_collate_key(e2->info.directory->path,-1);
		n = strcmp(key1,key2);
		g_free(key1);
		g_free(key2);
	}

	return n;
}

/* sort by list-format */
gint
compare_filelistentry_format(gconstpointer filelist_entry1,
			     gconstpointer filelist_entry2)
{
	const mpd_InfoEntity *e1, *e2;
	char key1[BUFSIZE], key2[BUFSIZE];
	int n = 0;

	e1 = ((const filelist_entry_t *)filelist_entry1)->entity;
	e2 = ((const filelist_entry_t *)filelist_entry2)->entity;

	if (e1 && e2 &&
	    e1->type == MPD_INFO_ENTITY_TYPE_SONG &&
	    e2->type == MPD_INFO_ENTITY_TYPE_SONG) {
		strfsong(key1, BUFSIZE, LIST_FORMAT, e1->info.song);
		strfsong(key2, BUFSIZE, LIST_FORMAT, e2->info.song);
		n = strcmp(key1,key2);
	}

	return n;
}


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

#ifndef NDEBUG
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
      if(  c->connection->errorCode == MPD_ACK_ERROR_PERMISSION )
	{
	  if(screen_auth(c) == 0) return 0;
	}
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
	playlist_init(&c->playlist);

	return c;
}

void
mpdclient_free(mpdclient_t *c)
{
	mpdclient_disconnect(c);

	mpdclient_playlist_free(&c->playlist);

	g_list_free(c->error_callbacks);
	g_list_free(c->playlist_callbacks);
	g_list_free(c->browse_callbacks);
	g_free(c);
}

gint
mpdclient_disconnect(mpdclient_t *c)
{
	if (c->connection)
		mpd_closeConnection(c->connection);
	c->connection = NULL;

	if (c->status)
		mpd_freeStatus(c->status);
	c->status = NULL;

	playlist_clear(&c->playlist);

	if (c->song)
		c->song = NULL;

	return 0;
}

gint
mpdclient_connect(mpdclient_t *c,
		  gchar *host,
		  gint port,
		  gfloat _timeout,
		  gchar *password)
{
	gint retval = 0;

	/* close any open connection */
	if( c->connection )
		mpdclient_disconnect(c);

	/* connect to MPD */
	c->connection = mpd_newConnection(host, port, _timeout);
	if( c->connection->error )
		return error_cb(c, c->connection->error,
				c->connection->errorStr);

	/* send password */
	if( password ) {
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

	if (MPD_ERROR(c))
		return -1;

	/* free the old status */
	if (c->status)
		mpd_freeStatus(c->status);

	/* retreive new status */
	mpd_sendStatusCommand(c->connection);
	c->status = mpd_getStatus(c->connection);
	if ((retval=mpdclient_finish_command(c)))
		return retval;
#ifndef NDEBUG
	if (c->status->error)
		D("status> %s\n", c->status->error);
#endif

	/* check if the playlist needs an update */
	if (c->playlist.id != c->status->playlist) {
		if (playlist_is_empty(&c->playlist))
			retval = mpdclient_playlist_update_changes(c);
		else
			retval = mpdclient_playlist_update(c);
	}

	/* update the current song */
	if (!c->song || c->status->songid != c->song->id) {
		c->song = playlist_get_song(c, c->status->song);
	}

	c->need_update = FALSE;

	return retval;
}


/****************************************************************************/
/*** MPD Commands  **********************************************************/
/****************************************************************************/

gint
mpdclient_cmd_play(mpdclient_t *c, gint idx)
{
#ifdef ENABLE_SONG_ID
	struct mpd_song *song = playlist_get_song(c, idx);

	D("Play id:%d\n", song ? song->id : -1);
	if (song)
		mpd_sendPlayIdCommand(c->connection, song->id);
	else
		mpd_sendPlayIdCommand(c->connection, MPD_PLAY_AT_BEGINNING);
#else
	mpd_sendPlayCommand(c->connection, idx);
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
mpdclient_cmd_add(mpdclient_t *c, struct mpd_song *song)
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
	playlist_append(&c->playlist, song);

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
mpdclient_cmd_delete(mpdclient_t *c, gint idx)
{
	gint retval = 0;
	struct mpd_song *song;

	if (idx < 0 || (guint)idx >= playlist_length(&c->playlist))
		return -1;

	song = playlist_get(&c->playlist, idx);

	/* send the delete command to mpd */
#ifdef ENABLE_SONG_ID
	D("Delete id:%d\n", song->id);
	mpd_sendDeleteIdCommand(c->connection, song->id);
#else
	mpd_sendDeleteCommand(c->connection, idx);
#endif
	if( (retval=mpdclient_finish_command(c)) )
		return retval;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_DELETE
	/* increment the playlist id, so we dont retrive a new playlist */
	c->playlist.id++;

	/* remove the song from the playlist */
	playlist_remove_reuse(&c->playlist, idx);

	/* call playlist updated callback */
	mpdclient_playlist_callback(c, PLAYLIST_EVENT_DELETE, (gpointer) song);

	/* remove references to the song */
	if (c->song == song) {
		c->song = NULL;
		c->need_update = TRUE;
	}

	mpd_freeSong(song);

#else
	c->need_update = TRUE;
#endif

	return 0;
}

gint
mpdclient_cmd_move(mpdclient_t *c, gint old_index, gint new_index)
{
	gint n;
	struct mpd_song *song1, *song2;

	if (old_index == new_index || new_index < 0 ||
	    (guint)new_index >= c->playlist.list->len)
		return -1;

	song1 = playlist_get(&c->playlist, old_index);
	song2 = playlist_get(&c->playlist, new_index);

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
	/* update the playlist */
	playlist_swap(&c->playlist, old_index, new_index);

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


/* update playlist */
gint
mpdclient_playlist_update(mpdclient_t *c)
{
	mpd_InfoEntity *entity;

	D("mpdclient_playlist_update() [%lld]\n", c->status->playlist);

	if (MPD_ERROR(c))
		return -1;

	playlist_clear(&c->playlist);

	mpd_sendPlaylistInfoCommand(c->connection,-1);
	while ((entity = mpd_getNextInfoEntity(c->connection))) {
		if (entity->type == MPD_INFO_ENTITY_TYPE_SONG)
			playlist_append(&c->playlist, entity->info.song);

		mpd_freeInfoEntity(entity);
	}

	c->playlist.id = c->status->playlist;
	c->song = NULL;

	/* call playlist updated callbacks */
	mpdclient_playlist_callback(c, PLAYLIST_EVENT_UPDATED, NULL);

	return mpdclient_finish_command(c);
}

#ifdef ENABLE_PLCHANGES

/* update playlist (plchanges) */
gint
mpdclient_playlist_update_changes(mpdclient_t *c)
{
	mpd_InfoEntity *entity;

	D("mpdclient_playlist_update_changes() [%lld -> %lld]\n",
	  c->status->playlist, c->playlist.id);

	if (MPD_ERROR(c))
		return -1;

	mpd_sendPlChangesCommand(c->connection, c->playlist.id);

	while ((entity = mpd_getNextInfoEntity(c->connection)) != NULL) {
		struct mpd_song *song = entity->info.song;

		if (song->pos >= 0 && (guint)song->pos < c->playlist.list->len) {
			/* update song */
			D("updating pos:%d, id=%d - %s\n",
			  song->pos, song->id, song->file);
			playlist_replace(&c->playlist, song->pos, song);
		} else {
			/* add a new song */
			D("adding song at pos %d\n", song->pos);
			playlist_append(&c->playlist, song);
		}

		mpd_freeInfoEntity(entity);
	}

	/* remove trailing songs */
	while ((guint)c->status->playlistLength < c->playlist.list->len) {
		guint pos = c->playlist.list->len - 1;

		/* Remove the last playlist entry */
		D("removing song at pos %d\n", pos);
		playlist_remove(&c->playlist, pos);
	}

	c->song = NULL;
	c->playlist.id = c->status->playlist;

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


/****************************************************************************/
/*** Filelist functions *****************************************************/
/****************************************************************************/

mpdclient_filelist_t *
mpdclient_filelist_get(mpdclient_t *c, const gchar *path)
{
	mpdclient_filelist_t *filelist;
	mpd_InfoEntity *entity;
	gchar *path_utf8 = locale_to_utf8(path);
	gboolean has_dirs_only = TRUE;

	D("mpdclient_filelist_get(%s)\n", path);
	mpd_sendLsInfoCommand(c->connection, path_utf8);
	filelist = filelist_new(path);
	if (path && path[0] && strcmp(path, "/"))
		/* add a dummy entry for ./.. */
		filelist_append(filelist, NULL);

	while ((entity=mpd_getNextInfoEntity(c->connection))) {
		filelist_append(filelist, entity);

		if (has_dirs_only && entity->type != MPD_INFO_ENTITY_TYPE_DIRECTORY) {
			has_dirs_only = FALSE;
		}
	}

	/* If there's an error, ignore it.  We'll return an empty filelist. */
	mpdclient_finish_command(c);

	g_free(path_utf8);
	filelist->updated = TRUE;

	// If there are only directory entities in the filelist, we sort it
	if (has_dirs_only) {
		D("mpdclient_filelist_get: only dirs; sorting!\n");
		filelist_sort(filelist, compare_filelistentry_dir);
	}

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
	if (exact_match)
		mpd_sendFindCommand(c->connection, table, filter_utf8);
	else
		mpd_sendSearchCommand(c->connection, table, filter_utf8);
	filelist = filelist_new(NULL);

	while ((entity=mpd_getNextInfoEntity(c->connection)))
		filelist_append(filelist, entity);

	if (mpdclient_finish_command(c)) {
		filelist_free(filelist);
		return NULL;
	}

	filelist->updated = TRUE;
	return filelist;
}


mpdclient_filelist_t *
mpdclient_filelist_search(mpdclient_t *c,
			  int exact_match,
			  int table,
			  gchar *_filter)
{
	mpdclient_filelist_t *filelist;
	gchar *filter_utf8 = locale_to_utf8(_filter);

	D("mpdclient_filelist_search(%s)\n", _filter);
	filelist = mpdclient_filelist_search_utf8(c, exact_match, table,
						  filter_utf8);
	g_free(filter_utf8);

	return filelist;
}

mpdclient_filelist_t *
mpdclient_filelist_update(mpdclient_t *c, mpdclient_filelist_t *filelist)
{
  if( filelist != NULL )
    {    
      gchar *path = g_strdup(filelist->path);

      filelist_free(filelist);
      filelist = mpdclient_filelist_get(c, path);
      g_free(path);
      return filelist;
    }
  return NULL;
}

int
mpdclient_filelist_add_all(mpdclient_t *c, mpdclient_filelist_t *fl)
{
	guint i;

	if (filelist_is_empty(fl))
		return 0;

	mpd_sendCommandListBegin(c->connection);

	for (i = 0; i < filelist_length(fl); ++i) {
		filelist_entry_t *entry = filelist_get(fl, i);
		mpd_InfoEntity *entity  = entry->entity;

		if (entity && entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			struct mpd_song *song = entity->info.song;

			mpd_sendAddCommand(c->connection, song->file);
		}
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





