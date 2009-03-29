#ifndef MPDCLIENT_H
#define MPDCLIENT_H

#include "libmpdclient.h"
#include "playlist.h"
#include "filelist.h"

#define MPD_VERSION_EQ(c,x,y,z) (c->connection->version[0] == x && \
                                 c->connection->version[1] == y && \
                                 c->connection->version[2] == z )

#define MPD_VERSION_LT(c,x,y,z) ( c->connection->version[0]<x  || \
 (c->connection->version[0]==x && c->connection->version[1]<y) || \
 (c->connection->version[0]==x && c->connection->version[1]==y && \
  c->connection->version[2]<z) )

typedef struct mpdclient {
	/* playlist */
	mpdclient_playlist_t playlist;

	/* Callbacks */
	GList *error_callbacks;
	GList *playlist_callbacks;
	GList *browse_callbacks;

	mpd_Connection *connection;
	mpd_Status     *status;
	mpd_Song       *song;

	gboolean       need_update;
} mpdclient_t;

/** functions ***************************************************************/

gint
mpdclient_finish_command(mpdclient_t *c);

mpdclient_t *mpdclient_new(void);
void mpdclient_free(mpdclient_t *c);
gint mpdclient_connect(mpdclient_t *c, gchar *host, gint port,
		       gfloat timeout_, gchar *password);
gint mpdclient_disconnect(mpdclient_t *c);
gint mpdclient_update(mpdclient_t *c);


/*** MPD Commands  **********************************************************/
gint mpdclient_cmd_play(mpdclient_t *c, gint index);
gint mpdclient_cmd_pause(mpdclient_t *c, gint value);
gint
mpdclient_cmd_crop(mpdclient_t *c);
gint mpdclient_cmd_stop(mpdclient_t *c);
gint mpdclient_cmd_next(mpdclient_t *c);
gint mpdclient_cmd_prev(mpdclient_t *c);
gint mpdclient_cmd_seek(mpdclient_t *c, gint id, gint pos);
gint mpdclient_cmd_shuffle(mpdclient_t *c);
gint mpdclient_cmd_shuffle_range(mpdclient_t *c, guint start, guint end);
gint mpdclient_cmd_clear(mpdclient_t *c);
gint mpdclient_cmd_repeat(mpdclient_t *c, gint value);
gint mpdclient_cmd_random(mpdclient_t *c, gint value);
gint mpdclient_cmd_single(mpdclient_t *c, gint value);
gint mpdclient_cmd_consume(mpdclient_t *c, gint value);
gint mpdclient_cmd_crossfade(mpdclient_t *c, gint value);
gint mpdclient_cmd_db_update(mpdclient_t *c, gchar *path);
gint mpdclient_cmd_volume(mpdclient_t *c, gint value);
gint mpdclient_cmd_add_path(mpdclient_t *c, gchar *path);

gint mpdclient_cmd_add(mpdclient_t *c, struct mpd_song *song);
gint mpdclient_cmd_delete(mpdclient_t *c, gint index);
gint mpdclient_cmd_move(mpdclient_t *c, gint old_index, gint new_index);

gint mpdclient_cmd_save_playlist(mpdclient_t *c, gchar *filename);
gint mpdclient_cmd_load_playlist(mpdclient_t *c, gchar *filename_utf8);
gint mpdclient_cmd_delete_playlist(mpdclient_t *c, gchar *filename_utf8);

/* list functions */
GList *mpdclient_get_artists(mpdclient_t *c);
GList *mpdclient_get_albums(mpdclient_t *c, gchar *artist_utf8);


/*** error callbacks *****************************************************/

#define IS_ACK_ERROR(n)       (n & MPD_ERROR_ACK)
#define GET_ACK_ERROR_CODE(n) ((n & 0xFF00) >> 8)

typedef void (*mpdc_error_cb_t) (mpdclient_t *c, gint error, const gchar *msg);

void mpdclient_install_error_callback(mpdclient_t *c, mpdc_error_cb_t cb);
void mpdclient_remove_error_callback(mpdclient_t *c, mpdc_error_cb_t cb);

/*** playlist functions  **************************************************/

/* update the complete playlist */
gint mpdclient_playlist_update(struct mpdclient *c);

/* get playlist changes */
gint mpdclient_playlist_update_changes(struct mpdclient *c);


/*** mpdclient playlist callbacks *****************************************/

#define PLAYLIST_EVENT_UPDATED     0x01
#define PLAYLIST_EVENT_CLEAR       0x02
#define PLAYLIST_EVENT_DELETE      0x03
#define PLAYLIST_EVENT_ADD         0x04
#define PLAYLIST_EVENT_MOVE        0x05


typedef void (*mpdc_list_cb_t) (mpdclient_t *c, int event, gpointer data);

/* install a playlist callback function */
void mpdclient_install_playlist_callback(mpdclient_t *c, mpdc_list_cb_t cb);

/* remove a playlist callback function */
void mpdclient_remove_playlist_callback(mpdclient_t *c, mpdc_list_cb_t cb);


/* issue a playlist callback */
void mpdclient_playlist_callback(mpdclient_t *c, int event, gpointer data);


/*** filelist functions  ***************************************************/

mpdclient_filelist_t *mpdclient_filelist_get(mpdclient_t *c, const gchar *path);
mpdclient_filelist_t *mpdclient_filelist_search(mpdclient_t *c,
						int exact_match,
						int table,
						gchar *filter_utf8);
mpdclient_filelist_t *mpdclient_filelist_update(mpdclient_t *c,
						mpdclient_filelist_t *flist);

/* add all songs in filelist to the playlist */
int mpdclient_filelist_add_all(mpdclient_t *c, mpdclient_filelist_t *fl);

/*** mpdclient browse callbacks ********************************************/

#define BROWSE_DB_UPDATED          0x01
#define BROWSE_PLAYLIST_SAVED      0x02
#define BROWSE_PLAYLIST_DELETED    0x03


/* install a playlist callback function */
void mpdclient_install_browse_callback(mpdclient_t *c, mpdc_list_cb_t cb);

/* remove a playlist callback function */
void mpdclient_remove_browse_callback(mpdclient_t *c, mpdc_list_cb_t cb);


/* issue a playlist callback */
void mpdclient_browse_callback(mpdclient_t *c, int event, gpointer data);

/* sort by list-format */
gint compare_filelistentry_format(gconstpointer filelist_entry1, gconstpointer filelist_entry2);

#endif
