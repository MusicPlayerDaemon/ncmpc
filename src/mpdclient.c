/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mpdclient.h"
#include "screen_utils.h"
#include "config.h"
#include "options.h"
#include "strfsong.h"
#include "utils.h"

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

static bool
MPD_ERROR(const struct mpdclient *client)
{
	return client->connection == NULL;
}

/* filelist sorting functions */
static gint
compare_filelistentry(gconstpointer filelist_entry1,
			  gconstpointer filelist_entry2)
{
	const mpd_InfoEntity *e1, *e2;
	int n = 0;

	e1 = ((const filelist_entry_t *)filelist_entry1)->entity;
	e2 = ((const filelist_entry_t *)filelist_entry2)->entity;

	if (e1 && e2 && e1->type == e2->type) {
		switch (e1->type) {
		case MPD_INFO_ENTITY_TYPE_DIRECTORY:
			n = g_utf8_collate(e1->info.directory->path,
					e2->info.directory->path);
			break;
		case MPD_INFO_ENTITY_TYPE_SONG:
			break;
		case MPD_INFO_ENTITY_TYPE_PLAYLISTFILE:
			n = g_utf8_collate(e1->info.playlistFile->path,
					e2->info.playlistFile->path);
		}
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
		strfsong(key1, BUFSIZE, options.list_format, e1->info.song);
		strfsong(key2, BUFSIZE, options.list_format, e2->info.song);
		n = strcmp(key1,key2);
	}

	return n;
}


/* Error callbacks */
static gint
error_cb(mpdclient_t *c, gint error, const gchar *msg)
{
	GList *list = c->error_callbacks;

	if (list == NULL)
		fprintf(stderr, "error [%d]: %s\n", (error & 0xFF), msg);

	while (list) {
		mpdc_error_cb_t cb = list->data;
		if (cb)
			cb(c, error, msg);
		list = list->next;
	}

	mpd_clearError(c->connection);
	return error;
}


/****************************************************************************/
/*** mpdclient functions ****************************************************/
/****************************************************************************/

static gint
mpdclient_handle_error(mpdclient_t *c)
{
	enum mpd_error error = c->connection->error;
	bool is_fatal = error != MPD_ERROR_ACK;

	if (error == MPD_ERROR_SUCCESS)
		return 0;

	if (error == MPD_ERROR_ACK &&
	    c->connection->errorCode == MPD_ACK_ERROR_PERMISSION &&
	    screen_auth(c) == 0)
		return 0;

	if (error == MPD_ERROR_ACK)
		error = error | (c->connection->errorCode << 8);

	error_cb(c, error, c->connection->errorStr);

	if (is_fatal)
		mpdclient_disconnect(c);

	return error;
}

gint
mpdclient_finish_command(mpdclient_t *c)
{
	mpd_finishCommand(c->connection);
	return mpdclient_handle_error(c);
}

mpdclient_t *
mpdclient_new(void)
{
	mpdclient_t *c;

	c = g_malloc0(sizeof(mpdclient_t));
	playlist_init(&c->playlist);
	c->volume = MPD_STATUS_NO_VOLUME;

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
		  const gchar *host,
		  gint port,
		  gfloat _timeout,
		  const gchar *password)
{
	gint retval = 0;

	/* close any open connection */
	if( c->connection )
		mpdclient_disconnect(c);

	/* connect to MPD */
	c->connection = mpd_newConnection(host, port, _timeout);
	if (c->connection->error) {
		retval = error_cb(c, c->connection->error,
				  c->connection->errorStr);
		if (retval != 0) {
			mpd_closeConnection(c->connection);
			c->connection = NULL;
		}

		return retval;
	}

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

	c->volume = MPD_STATUS_NO_VOLUME;

	if (MPD_ERROR(c))
		return -1;

	/* free the old status */
	if (c->status)
		mpd_freeStatus(c->status);

	/* retrieve new status */
	mpd_sendStatusCommand(c->connection);
	c->status = mpd_getStatus(c->connection);
	if ((retval=mpdclient_finish_command(c)))
		return retval;

	if (c->updatingdb && c->updatingdb != c->status->updatingDb)
		mpdclient_browse_callback(c, BROWSE_DB_UPDATED, NULL);

	c->updatingdb = c->status->updatingDb;
	c->volume = c->status->volume;

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

	if (MPD_ERROR(c))
		return -1;

	if (song)
		mpd_sendPlayIdCommand(c->connection, song->id);
	else
		mpd_sendPlayIdCommand(c->connection, MPD_PLAY_AT_BEGINNING);
#else
	if (MPD_ERROR(c))
		return -1;

	mpd_sendPlayCommand(c->connection, idx);
#endif
	c->need_update = TRUE;
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_pause(mpdclient_t *c, gint value)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendPauseCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_crop(mpdclient_t *c)
{
	gint error;
	mpd_Status *status;
	bool playing;
	int length, current;

	if (MPD_ERROR(c))
		return -1;

	mpd_sendStatusCommand(c->connection);
	status = mpd_getStatus(c->connection);
	error = mpdclient_finish_command(c);
	if (error)
		return error;

	playing = status->state == MPD_STATUS_STATE_PLAY ||
		status->state == MPD_STATUS_STATE_PAUSE;
	length = status->playlistLength;
	current = status->song;

	mpd_freeStatus(status);

	if (!playing || length < 2)
		return 0;

	mpd_sendCommandListBegin( c->connection );

	while (--length >= 0)
		if (length != current)
			mpd_sendDeleteCommand(c->connection, length);

	mpd_sendCommandListEnd(c->connection);

	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_stop(mpdclient_t *c)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendStopCommand(c->connection);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_next(mpdclient_t *c)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendNextCommand(c->connection);
	c->need_update = TRUE;
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_prev(mpdclient_t *c)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendPrevCommand(c->connection);
	c->need_update = TRUE;
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_seek(mpdclient_t *c, gint id, gint pos)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendSeekIdCommand(c->connection, id, pos);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_shuffle(mpdclient_t *c)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendShuffleCommand(c->connection);
	c->need_update = TRUE;
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_shuffle_range(mpdclient_t *c, guint start, guint end)
{
	mpd_sendShuffleRangeCommand(c->connection, start, end);
	c->need_update = TRUE;
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_clear(mpdclient_t *c)
{
	gint retval = 0;

	if (MPD_ERROR(c))
		return -1;

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
	if (MPD_ERROR(c))
		return -1;

	mpd_sendRepeatCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_random(mpdclient_t *c, gint value)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendRandomCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_single(mpdclient_t *c, gint value)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendSingleCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_consume(mpdclient_t *c, gint value)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendConsumeCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_crossfade(mpdclient_t *c, gint value)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendCrossfadeCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_db_update(mpdclient_t *c, const gchar *path)
{
	gint ret;

	if (MPD_ERROR(c))
		return -1;

	mpd_sendUpdateCommand(c->connection, path ? path : "");
	ret = mpdclient_finish_command(c);

	if (ret == 0)
		/* set updatingDb to make sure the browse callback
		   gets called even if the update has finished before
		   status is updated */
		c->updatingdb = 1;

	return ret;
}

gint
mpdclient_cmd_volume(mpdclient_t *c, gint value)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendSetvolCommand(c->connection, value);
	return mpdclient_finish_command(c);
}

gint mpdclient_cmd_volume_up(struct mpdclient *c)
{
	if (MPD_ERROR(c))
		return -1;

	if (c->status == NULL || c->status->volume == MPD_STATUS_NO_VOLUME)
		return 0;

	if (c->volume == MPD_STATUS_NO_VOLUME)
		c->volume = c->status->volume;

	if (c->volume >= 100)
		return 0;

	return mpdclient_cmd_volume(c, ++c->volume);
}

gint mpdclient_cmd_volume_down(struct mpdclient *c)
{
	if (MPD_ERROR(c))
		return -1;

	if (c->status == NULL || c->status->volume == MPD_STATUS_NO_VOLUME)
		return 0;

	if (c->volume == MPD_STATUS_NO_VOLUME)
		c->volume = c->status->volume;

	if (c->volume <= 0)
		return 0;

	return mpdclient_cmd_volume(c, --c->volume);
}

gint
mpdclient_cmd_add_path(mpdclient_t *c, const gchar *path_utf8)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendAddCommand(c->connection, path_utf8);
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_add(mpdclient_t *c, const struct mpd_song *song)
{
	gint retval = 0;

	if (MPD_ERROR(c))
		return -1;

	if( !song || !song->file )
		return -1;

	/* send the add command to mpd */
	mpd_sendAddCommand(c->connection, song->file);
	if( (retval=mpdclient_finish_command(c)) )
		return retval;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_ADD
	/* add the song to playlist */
	playlist_append(&c->playlist, song);

	/* increment the playlist id, so we don't retrieve a new playlist */
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

	if (MPD_ERROR(c))
		return -1;

	if (idx < 0 || (guint)idx >= playlist_length(&c->playlist))
		return -1;

	song = playlist_get(&c->playlist, idx);

	/* send the delete command to mpd */
#ifdef ENABLE_SONG_ID
	mpd_sendDeleteIdCommand(c->connection, song->id);
#else
	mpd_sendDeleteCommand(c->connection, idx);
#endif
	if( (retval=mpdclient_finish_command(c)) )
		return retval;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_DELETE
	/* increment the playlist id, so we don't retrieve a new playlist */
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

	if (MPD_ERROR(c))
		return -1;

	if (old_index == new_index || new_index < 0 ||
	    (guint)new_index >= c->playlist.list->len)
		return -1;

	song1 = playlist_get(&c->playlist, old_index);
	song2 = playlist_get(&c->playlist, new_index);

	/* send the move command to mpd */
#ifdef ENABLE_SONG_ID
	mpd_sendSwapIdCommand(c->connection, song1->id, song2->id);
#else
	mpd_sendMoveCommand(c->connection, old_index, new_index);
#endif
	if( (n=mpdclient_finish_command(c)) )
		return n;

#ifdef ENABLE_FANCY_PLAYLIST_MANAGMENT_CMD_MOVE
	/* update the playlist */
	playlist_swap(&c->playlist, old_index, new_index);

	/* increment the playlist id, so we don't retrieve a new playlist */
	c->playlist.id++;

#else
	c->need_update = TRUE;
#endif

	/* call playlist updated callback */
	mpdclient_playlist_callback(c, PLAYLIST_EVENT_MOVE, (gpointer) &new_index);

	return 0;
}

gint
mpdclient_cmd_save_playlist(mpdclient_t *c, const gchar *filename_utf8)
{
	gint retval = 0;

	if (MPD_ERROR(c))
		return -1;

	mpd_sendSaveCommand(c->connection, filename_utf8);
	if ((retval = mpdclient_finish_command(c)) == 0)
		mpdclient_browse_callback(c, BROWSE_PLAYLIST_SAVED, NULL);
	return retval;
}

gint
mpdclient_cmd_load_playlist(mpdclient_t *c, const gchar *filename_utf8)
{
	if (MPD_ERROR(c))
		return -1;

	mpd_sendLoadCommand(c->connection, filename_utf8);
	c->need_update = TRUE;
	return mpdclient_finish_command(c);
}

gint
mpdclient_cmd_delete_playlist(mpdclient_t *c, const gchar *filename_utf8)
{
	gint retval = 0;

	if (MPD_ERROR(c))
		return -1;

	mpd_sendRmCommand(c->connection, filename_utf8);
	if ((retval = mpdclient_finish_command(c)) == 0)
		mpdclient_browse_callback(c, BROWSE_PLAYLIST_DELETED, NULL);
	return retval;
}


/****************************************************************************/
/*** Callback management functions ******************************************/
/****************************************************************************/

static void
do_list_callbacks(mpdclient_t *c, GList *list, gint event, gpointer data)
{
	while (list) {
		mpdc_list_cb_t fn = list->data;

		fn(c, event, data);
		list = list->next;
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
/*** Playlist management functions ******************************************/
/****************************************************************************/

/* update playlist */
gint
mpdclient_playlist_update(mpdclient_t *c)
{
	mpd_InfoEntity *entity;

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

	if (MPD_ERROR(c))
		return -1;

	mpd_sendPlChangesCommand(c->connection, c->playlist.id);

	while ((entity = mpd_getNextInfoEntity(c->connection)) != NULL) {
		struct mpd_song *song = entity->info.song;

		if (song->pos >= 0 && (guint)song->pos < c->playlist.list->len) {
			/* update song */
			playlist_replace(&c->playlist, song->pos, song);
		} else {
			/* add a new song */
			playlist_append(&c->playlist, song);
		}

		mpd_freeInfoEntity(entity);
	}

	/* remove trailing songs */
	while ((guint)c->status->playlistLength < c->playlist.list->len) {
		guint pos = c->playlist.list->len - 1;

		/* Remove the last playlist entry */
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

	if (MPD_ERROR(c))
		return NULL;

	mpd_sendLsInfoCommand(c->connection, path);
	filelist = filelist_new();
	if (path && path[0] && strcmp(path, "/"))
		/* add a dummy entry for ./.. */
		filelist_append(filelist, NULL);

	while ((entity=mpd_getNextInfoEntity(c->connection))) {
		filelist_append(filelist, entity);
	}

	/* If there's an error, ignore it.  We'll return an empty filelist. */
	mpdclient_finish_command(c);

	filelist_sort_dir_play(filelist, compare_filelistentry);

	return filelist;
}

mpdclient_filelist_t *
mpdclient_filelist_search(mpdclient_t *c,
			  int exact_match,
			  int table,
			  gchar *filter_utf8)
{
	mpdclient_filelist_t *filelist;
	mpd_InfoEntity *entity;

	if (MPD_ERROR(c))
		return NULL;

	if (exact_match)
		mpd_sendFindCommand(c->connection, table, filter_utf8);
	else
		mpd_sendSearchCommand(c->connection, table, filter_utf8);
	filelist = filelist_new();

	while ((entity=mpd_getNextInfoEntity(c->connection)))
		filelist_append(filelist, entity);

	if (mpdclient_finish_command(c)) {
		filelist_free(filelist);
		return NULL;
	}

	return filelist;
}

int
mpdclient_filelist_add_all(mpdclient_t *c, mpdclient_filelist_t *fl)
{
	guint i;

	if (MPD_ERROR(c))
		return -1;

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
mpdclient_get_artists(mpdclient_t *c)
{
	gchar *str = NULL;
	GList *list = NULL;

	if (MPD_ERROR(c))
               return NULL;

	mpd_sendListCommand(c->connection, MPD_TABLE_ARTIST, NULL);
	while ((str = mpd_getNextArtist(c->connection)))
		list = g_list_append(list, (gpointer) str);

	if (mpdclient_finish_command(c))
		return string_list_free(list);

	return list;
}

GList *
mpdclient_get_albums(mpdclient_t *c, const gchar *artist_utf8)
{
	gchar *str = NULL;
	GList *list = NULL;

	if (MPD_ERROR(c))
               return NULL;

	mpd_sendListCommand(c->connection, MPD_TABLE_ALBUM, artist_utf8);
	while ((str = mpd_getNextAlbum(c->connection)))
		list = g_list_append(list, (gpointer) str);

	if (mpdclient_finish_command(c))
		return string_list_free(list);

	return list;
}
