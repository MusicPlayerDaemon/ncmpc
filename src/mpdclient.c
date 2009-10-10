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
#include "filelist.h"
#include "screen_client.h"
#include "config.h"
#include "options.h"
#include "strfsong.h"
#include "utils.h"
#include "gidle.h"

#include <mpd/client.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define BUFSIZE 1024

/* sort by list-format */
gint
compare_filelistentry_format(gconstpointer filelist_entry1,
			     gconstpointer filelist_entry2)
{
	const struct mpd_entity *e1, *e2;
	char key1[BUFSIZE], key2[BUFSIZE];
	int n = 0;

	e1 = ((const struct filelist_entry *)filelist_entry1)->entity;
	e2 = ((const struct filelist_entry *)filelist_entry2)->entity;

	if (e1 && e2 &&
	    mpd_entity_get_type(e1) == MPD_ENTITY_TYPE_SONG &&
	    mpd_entity_get_type(e2) == MPD_ENTITY_TYPE_SONG) {
		strfsong(key1, BUFSIZE, options.list_format, mpd_entity_get_song(e1));
		strfsong(key2, BUFSIZE, options.list_format, mpd_entity_get_song(e2));
		n = strcmp(key1,key2);
	}

	return n;
}


/****************************************************************************/
/*** mpdclient functions ****************************************************/
/****************************************************************************/

bool
mpdclient_handle_error(struct mpdclient *c)
{
	enum mpd_error error = mpd_connection_get_error(c->connection);

	assert(error != MPD_ERROR_SUCCESS);

	if (error == MPD_ERROR_SERVER &&
	    mpd_connection_get_server_error(c->connection) == MPD_SERVER_ERROR_PERMISSION &&
	    screen_auth(c))
		return true;

	mpdclient_ui_error(mpd_connection_get_error_message(c->connection));

	if (!mpd_connection_clear_error(c->connection))
		mpdclient_disconnect(c);

	return false;
}

static bool
mpdclient_finish_command(struct mpdclient *c)
{
	return mpd_response_finish(c->connection)
		? true : mpdclient_handle_error(c);
}

struct mpdclient *
mpdclient_new(void)
{
	struct mpdclient *c;

	c = g_new0(struct mpdclient, 1);
	playlist_init(&c->playlist);
	c->volume = -1;
	c->events = 0;

	return c;
}

void
mpdclient_free(struct mpdclient *c)
{
	mpdclient_disconnect(c);

	mpdclient_playlist_free(&c->playlist);

	g_free(c);
}

void
mpdclient_disconnect(struct mpdclient *c)
{
	if (c->source != NULL) {
		mpd_glib_free(c->source);
		c->source = NULL;
		c->idle = false;
	}

	if (c->connection)
		mpd_connection_free(c->connection);
	c->connection = NULL;

	if (c->status)
		mpd_status_free(c->status);
	c->status = NULL;

	playlist_clear(&c->playlist);

	if (c->song)
		c->song = NULL;

	/* everything has changed after a disconnect */
	c->events |= MPD_IDLE_DATABASE|MPD_IDLE_STORED_PLAYLIST|
		MPD_IDLE_QUEUE|MPD_IDLE_PLAYER|MPD_IDLE_MIXER|MPD_IDLE_OUTPUT|
		MPD_IDLE_OPTIONS|MPD_IDLE_UPDATE;
}

bool
mpdclient_connect(struct mpdclient *c,
		  const gchar *host,
		  gint port,
		  gfloat _timeout,
		  const gchar *password)
{
	/* close any open connection */
	if( c->connection )
		mpdclient_disconnect(c);

	/* connect to MPD */
	c->connection = mpd_connection_new(host, port, _timeout * 1000);
	if (c->connection == NULL)
		g_error("Out of memory");

	if (mpd_connection_get_error(c->connection) != MPD_ERROR_SUCCESS) {
		mpdclient_handle_error(c);
		mpdclient_disconnect(c);
		return false;
	}

	/* send password */
	if (password != NULL && !mpd_run_password(c->connection, password)) {
		mpdclient_handle_error(c);
		mpdclient_disconnect(c);
		return false;
	}

	return true;
}

bool
mpdclient_update(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	bool retval;

	c->volume = -1;

	if (connection == NULL)
		return false;

	/* always announce these options as long as we don't have
	   "idle" support */
	if (c->source == NULL)
		c->events |= MPD_IDLE_PLAYER|MPD_IDLE_OPTIONS;

	/* free the old status */
	if (c->status)
		mpd_status_free(c->status);

	/* retrieve new status */
	c->status = mpd_run_status(connection);
	if (c->status == NULL)
		return mpdclient_handle_error(c);

	if (c->source == NULL &&
	    c->update_id != mpd_status_get_update_id(c->status)) {
		c->events |= MPD_IDLE_UPDATE;

		if (c->update_id > 0)
			c->events |= MPD_IDLE_DATABASE;
	}

	c->update_id = mpd_status_get_update_id(c->status);

	if (c->source == NULL &&
	    c->volume != mpd_status_get_volume(c->status))
		c->events |= MPD_IDLE_MIXER;

	c->volume = mpd_status_get_volume(c->status);

	/* check if the playlist needs an update */
	if (c->playlist.version != mpd_status_get_queue_version(c->status)) {
		if (c->source == NULL)
			c->events |= MPD_IDLE_PLAYLIST;

		if (!playlist_is_empty(&c->playlist))
			retval = mpdclient_playlist_update_changes(c);
		else
			retval = mpdclient_playlist_update(c);
	} else
		retval = true;

	/* update the current song */
	if (!c->song || mpd_status_get_song_id(c->status)) {
		c->song = playlist_get_song(&c->playlist,
					    mpd_status_get_song_pos(c->status));
	}

	return retval;
}

struct mpd_connection *
mpdclient_get_connection(struct mpdclient *c)
{
	if (c->source != NULL && c->idle) {
		c->idle = false;
		mpd_glib_leave(c->source);
	}

	return c->connection;
}

void
mpdclient_put_connection(struct mpdclient *c)
{
	assert(c->source == NULL || c->connection != NULL);

	if (c->source != NULL && !c->idle) {
		c->idle = true;
		mpd_glib_enter(c->source);
	}
}


/****************************************************************************/
/*** MPD Commands  **********************************************************/
/****************************************************************************/

bool
mpdclient_cmd_play(struct mpdclient *c, gint idx)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	const struct mpd_song *song = playlist_get_song(&c->playlist, idx);

	if (connection == NULL)
		return false;

	if (song)
		mpd_send_play_id(connection, mpd_song_get_id(song));
	else
		mpd_send_play(connection);

	return mpdclient_finish_command(c);
}

bool
mpdclient_cmd_crop(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	struct mpd_status *status;
	bool playing;
	int length, current;

	if (connection == NULL)
		return false;

	status = mpd_run_status(connection);
	if (status == NULL)
		return mpdclient_handle_error(c);

	playing = mpd_status_get_state(status) == MPD_STATE_PLAY ||
		mpd_status_get_state(status) == MPD_STATE_PAUSE;
	length = mpd_status_get_queue_length(status);
	current = mpd_status_get_song_pos(status);

	mpd_status_free(status);

	if (!playing || length < 2)
		return true;

	mpd_command_list_begin(connection, false);

	while (--length >= 0)
		if (length != current)
			mpd_send_delete(connection, length);

	mpd_command_list_end(connection);

	return mpdclient_finish_command(c);
}

bool
mpdclient_cmd_clear(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	bool retval;

	if (connection == NULL)
		return false;

	mpd_send_clear(connection);
	retval = mpdclient_finish_command(c);

	if (retval)
		c->events |= MPD_IDLE_PLAYLIST;

	return retval;
}

bool
mpdclient_cmd_volume(struct mpdclient *c, gint value)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	mpd_send_set_volume(connection, value);
	return mpdclient_finish_command(c);
}

bool
mpdclient_cmd_volume_up(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	if (c->status == NULL ||
	    mpd_status_get_volume(c->status) == -1)
		return true;

	if (c->volume < 0)
		c->volume = mpd_status_get_volume(c->status);

	if (c->volume >= 100)
		return true;

	return mpdclient_cmd_volume(c, ++c->volume);
}

bool
mpdclient_cmd_volume_down(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	if (c->status == NULL || mpd_status_get_volume(c->status) < 0)
		return true;

	if (c->volume < 0)
		c->volume = mpd_status_get_volume(c->status);

	if (c->volume <= 0)
		return true;

	return mpdclient_cmd_volume(c, --c->volume);
}

bool
mpdclient_cmd_add_path(struct mpdclient *c, const gchar *path_utf8)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	mpd_send_add(connection, path_utf8);
	return mpdclient_finish_command(c);
}

bool
mpdclient_cmd_add(struct mpdclient *c, const struct mpd_song *song)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	struct mpd_status *status;
	struct mpd_song *new_song;

	assert(c != NULL);
	assert(song != NULL);

	if (connection == NULL || c->status == NULL)
		return false;

	/* send the add command to mpd; at the same time, get the new
	   status (to verify the new playlist id) and the last song
	   (we hope that's the song we just added) */

	if (!mpd_command_list_begin(connection, true) ||
	    !mpd_send_add(connection, mpd_song_get_uri(song)) ||
	    !mpd_send_status(connection) ||
	    !mpd_send_get_queue_song_pos(connection,
					 playlist_length(&c->playlist)) ||
	    !mpd_command_list_end(connection) ||
	    !mpd_response_next(connection))
		return mpdclient_handle_error(c);

	c->events |= MPD_IDLE_PLAYLIST;

	status = mpd_recv_status(connection);
	if (status != NULL) {
		if (c->status != NULL)
			mpd_status_free(c->status);
		c->status = status;
	}

	if (!mpd_response_next(connection))
		return mpdclient_handle_error(c);

	new_song = mpd_recv_song(connection);
	if (!mpd_response_finish(connection) || new_song == NULL) {
		if (new_song != NULL)
			mpd_song_free(new_song);

		return mpd_connection_clear_error(connection) ||
			mpdclient_handle_error(c);
	}

	if (mpd_status_get_queue_length(status) == playlist_length(&c->playlist) + 1 &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* the song we just received has the correct id;
		   append it to the local playlist */
		playlist_append(&c->playlist, new_song);
	}

	mpd_song_free(new_song);

	return true;
}

bool
mpdclient_cmd_delete(struct mpdclient *c, gint idx)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	const struct mpd_song *song;
	struct mpd_status *status;

	if (connection == NULL || c->status == NULL)
		return false;

	if (idx < 0 || (guint)idx >= playlist_length(&c->playlist))
		return false;

	song = playlist_get(&c->playlist, idx);

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_delete_id(connection, mpd_song_get_id(song)) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return mpdclient_handle_error(c);

	c->events |= MPD_IDLE_PLAYLIST;

	status = mpd_recv_status(connection);
	if (status != NULL) {
		if (c->status != NULL)
			mpd_status_free(c->status);
		c->status = status;
	}

	if (!mpd_response_finish(connection))
		return mpdclient_handle_error(c);

	if (mpd_status_get_queue_length(status) == playlist_length(&c->playlist) - 1 &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* remove the song from the local playlist */
		playlist_remove(&c->playlist, idx);

		/* remove references to the song */
		if (c->song == song)
			c->song = NULL;
	}

	return true;
}

/**
 * Fallback for mpdclient_cmd_delete_range() on MPD older than 0.16.
 * It emulates the "delete range" command with a list of simple
 * "delete" commands.
 */
static bool
mpdclient_cmd_delete_range_fallback(struct mpdclient *c,
				    unsigned start, unsigned end)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	if (!mpd_command_list_begin(connection, false))
		return mpdclient_handle_error(c);

	for (; start < end; --end)
		mpd_send_delete(connection, start);

	if (!mpd_command_list_end(connection) ||
	    !mpd_response_finish(connection))
		return mpdclient_handle_error(c);

	return true;
}

bool
mpdclient_cmd_delete_range(struct mpdclient *c, unsigned start, unsigned end)
{
	struct mpd_connection *connection;
	struct mpd_status *status;

	connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	if (mpd_connection_cmp_server_version(connection, 0, 16, 0) < 0)
		return mpdclient_cmd_delete_range_fallback(c, start, end);

	/* MPD 0.16 supports "delete" with a range argument */

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_delete_range(connection, start, end) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return mpdclient_handle_error(c);

	c->events |= MPD_IDLE_PLAYLIST;

	status = mpd_recv_status(connection);
	if (status != NULL) {
		if (c->status != NULL)
			mpd_status_free(c->status);
		c->status = status;
	}

	if (!mpd_response_finish(connection))
		return mpdclient_handle_error(c);

	if (mpd_status_get_queue_length(status) == playlist_length(&c->playlist) - (end - start) &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* remove the song from the local playlist */
		while (end > start) {
			--end;

			/* remove references to the song */
			if (c->song == playlist_get(&c->playlist, end))
				c->song = NULL;

			playlist_remove(&c->playlist, end);
		}
	}

	return true;
}

bool
mpdclient_cmd_move(struct mpdclient *c, gint old_index, gint new_index)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	const struct mpd_song *song1, *song2;
	struct mpd_status *status;

	if (connection == NULL)
		return false;

	if (old_index == new_index || new_index < 0 ||
	    (guint)new_index >= c->playlist.list->len)
		return false;

	song1 = playlist_get(&c->playlist, old_index);
	song2 = playlist_get(&c->playlist, new_index);

	/* send the delete command to mpd; at the same time, get the
	   new status (to verify the playlist id) */

	if (!mpd_command_list_begin(connection, false) ||
	    !mpd_send_swap_id(connection, mpd_song_get_id(song1),
			      mpd_song_get_id(song2)) ||
	    !mpd_send_status(connection) ||
	    !mpd_command_list_end(connection))
		return mpdclient_handle_error(c);

	c->events |= MPD_IDLE_PLAYLIST;

	status = mpd_recv_status(connection);
	if (status != NULL) {
		if (c->status != NULL)
			mpd_status_free(c->status);
		c->status = status;
	}

	if (!mpd_response_finish(connection))
		return mpdclient_handle_error(c);

	if (mpd_status_get_queue_length(status) == playlist_length(&c->playlist) &&
	    mpd_status_get_queue_version(status) == c->playlist.version + 1) {
		/* the cheap route: match on the new playlist length
		   and its version, we can keep our local playlist
		   copy in sync */
		c->playlist.version = mpd_status_get_queue_version(status);

		/* swap songs in the local playlist */
		playlist_swap(&c->playlist, old_index, new_index);
	}

	return true;
}


/****************************************************************************/
/*** Playlist management functions ******************************************/
/****************************************************************************/

/* update playlist */
bool
mpdclient_playlist_update(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	struct mpd_entity *entity;

	if (connection == NULL)
		return false;

	playlist_clear(&c->playlist);

	mpd_send_list_queue_meta(connection);
	while ((entity = mpd_recv_entity(connection))) {
		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
			playlist_append(&c->playlist, mpd_entity_get_song(entity));

		mpd_entity_free(entity);
	}

	c->playlist.version = mpd_status_get_queue_version(c->status);
	c->song = NULL;

	return mpdclient_finish_command(c);
}

/* update playlist (plchanges) */
bool
mpdclient_playlist_update_changes(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	struct mpd_song *song;
	guint length;

	if (connection == NULL)
		return false;

	mpd_send_queue_changes_meta(connection, c->playlist.version);

	while ((song = mpd_recv_song(connection)) != NULL) {
		int pos = mpd_song_get_pos(song);

		if (pos >= 0 && (guint)pos < c->playlist.list->len) {
			/* update song */
			playlist_replace(&c->playlist, pos, song);
		} else {
			/* add a new song */
			playlist_append(&c->playlist, song);
		}

		mpd_song_free(song);
	}

	/* remove trailing songs */

	length = mpd_status_get_queue_length(c->status);
	while (length < c->playlist.list->len) {
		guint pos = c->playlist.list->len - 1;

		/* Remove the last playlist entry */
		playlist_remove(&c->playlist, pos);
	}

	c->song = NULL;
	c->playlist.version = mpd_status_get_queue_version(c->status);

	return mpdclient_finish_command(c);
}


/****************************************************************************/
/*** Filelist functions *****************************************************/
/****************************************************************************/

bool
mpdclient_filelist_add_all(struct mpdclient *c, struct filelist *fl)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	guint i;

	if (connection == NULL)
		return false;

	if (filelist_is_empty(fl))
		return true;

	mpd_command_list_begin(connection, false);

	for (i = 0; i < filelist_length(fl); ++i) {
		struct filelist_entry *entry = filelist_get(fl, i);
		struct mpd_entity *entity  = entry->entity;

		if (entity != NULL &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const struct mpd_song *song =
				mpd_entity_get_song(entity);

			mpd_send_add(connection, mpd_song_get_uri(song));
		}
	}

	mpd_command_list_end(connection);
	return mpdclient_finish_command(c);
}
