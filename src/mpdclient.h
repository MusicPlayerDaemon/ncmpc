#ifndef MPDCLIENT_H
#define MPDCLIENT_H

#include "playlist.h"

#include <mpd/client.h>

struct filelist;

struct mpdclient {
	/* playlist */
	struct mpdclient_playlist playlist;

	/* Callbacks */
	GList *error_callbacks;
	GList *playlist_callbacks;
	GList *browse_callbacks;

	struct mpd_connection *connection;
	struct mpd_status *status;
	const struct mpd_song *song;

	int volume;
	unsigned update_id;
};

/** functions ***************************************************************/

gint
mpdclient_handle_error(struct mpdclient *c);

struct mpdclient *
mpdclient_new(void);

void mpdclient_free(struct mpdclient *c);

static inline bool
mpdclient_is_connected(const struct mpdclient *c)
{
	return c->connection != NULL;
}

bool
mpdclient_connect(struct mpdclient *c, const gchar *host, gint port,
		  gfloat timeout_, const gchar *password);

void
mpdclient_disconnect(struct mpdclient *c);

bool
mpdclient_update(struct mpdclient *c);


/*** MPD Commands  **********************************************************/
gint mpdclient_cmd_play(struct mpdclient *c, gint index);
gint
mpdclient_cmd_crop(struct mpdclient *c);
gint mpdclient_cmd_shuffle_range(struct mpdclient *c, guint start, guint end);
gint mpdclient_cmd_clear(struct mpdclient *c);
gint mpdclient_cmd_volume(struct mpdclient *c, gint value);
gint mpdclient_cmd_volume_up(struct mpdclient *c);
gint mpdclient_cmd_volume_down(struct mpdclient *c);
gint mpdclient_cmd_add_path(struct mpdclient *c, const gchar *path);

gint mpdclient_cmd_add(struct mpdclient *c, const struct mpd_song *song);
gint mpdclient_cmd_delete(struct mpdclient *c, gint index);
gint mpdclient_cmd_move(struct mpdclient *c, gint old_index, gint new_index);

gint mpdclient_cmd_save_playlist(struct mpdclient *c, const gchar *filename);
gint mpdclient_cmd_load_playlist(struct mpdclient *c, const gchar *filename_utf8);
gint mpdclient_cmd_delete_playlist(struct mpdclient *c, const gchar *filename_utf8);

/* list functions */
GList *mpdclient_get_artists(struct mpdclient *c);
GList *mpdclient_get_albums(struct mpdclient *c, const gchar *artist_utf8);


/*** error callbacks *****************************************************/

#define IS_ACK_ERROR(n)       (n & MPD_ERROR_ACK)
#define GET_ACK_ERROR_CODE(n) ((n & 0xFF00) >> 8)

typedef void (*mpdc_error_cb_t) (struct mpdclient *c, gint error, const gchar *msg);

void mpdclient_install_error_callback(struct mpdclient *c, mpdc_error_cb_t cb);
void mpdclient_remove_error_callback(struct mpdclient *c, mpdc_error_cb_t cb);

/*** playlist functions  **************************************************/

/* update the complete playlist */
bool
mpdclient_playlist_update(struct mpdclient *c);

/* get playlist changes */
bool
mpdclient_playlist_update_changes(struct mpdclient *c);


/*** mpdclient playlist callbacks *****************************************/

#define PLAYLIST_EVENT_UPDATED     0x01
#define PLAYLIST_EVENT_CLEAR       0x02
#define PLAYLIST_EVENT_DELETE      0x03
#define PLAYLIST_EVENT_ADD         0x04
#define PLAYLIST_EVENT_MOVE        0x05


typedef void (*mpdc_list_cb_t) (struct mpdclient *c, int event, gpointer data);

/* install a playlist callback function */
void mpdclient_install_playlist_callback(struct mpdclient *c, mpdc_list_cb_t cb);

/* remove a playlist callback function */
void mpdclient_remove_playlist_callback(struct mpdclient *c, mpdc_list_cb_t cb);


/* issue a playlist callback */
void mpdclient_playlist_callback(struct mpdclient *c, int event, gpointer data);


/*** filelist functions  ***************************************************/

struct filelist *
mpdclient_filelist_get(struct mpdclient *c, const gchar *path);

struct filelist *
mpdclient_filelist_search(struct mpdclient *c, int exact_match,
			  enum mpd_tag_type tag,
			  gchar *filter_utf8);

/* add all songs in filelist to the playlist */
int
mpdclient_filelist_add_all(struct mpdclient *c, struct filelist *fl);

/*** mpdclient browse callbacks ********************************************/

#define BROWSE_DB_UPDATED          0x01
#define BROWSE_PLAYLIST_SAVED      0x02
#define BROWSE_PLAYLIST_DELETED    0x03


/* install a playlist callback function */
void mpdclient_install_browse_callback(struct mpdclient *c, mpdc_list_cb_t cb);

/* remove a playlist callback function */
void mpdclient_remove_browse_callback(struct mpdclient *c, mpdc_list_cb_t cb);


/* issue a playlist callback */
void mpdclient_browse_callback(struct mpdclient *c, int event, gpointer data);

/* sort by list-format */
gint compare_filelistentry_format(gconstpointer filelist_entry1, gconstpointer filelist_entry2);

#endif
