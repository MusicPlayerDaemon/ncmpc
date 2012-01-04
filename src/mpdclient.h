#ifndef MPDCLIENT_H
#define MPDCLIENT_H

#include "playlist.h"
#include "mpdclient.h"

#include <mpd/client.h>

struct filelist;

struct mpdclient {
	/* playlist */
	struct mpdclient_playlist playlist;

	struct mpd_connection *connection;

	/**
	 * This attribute is incremented whenever the connection changes
	 * (i.e. on disconnection and (re-)connection).
	 */
	unsigned connection_id;

	/**
	 * If this object is non-NULL, it tracks idle events.  It is
	 * automatically called by mpdclient_get_connection() and
	 * mpdclient_put_connection().  It is not created by the
	 * mpdclient library; the user of this library has to
	 * initialize it.  However, it is freed when the MPD
	 * connection is closed.
	 */
	struct mpd_glib_source *source;

	/**
	 * This attribute is true when the connection is currently in
	 * "idle" mode, and the #mpd_glib_source waits for an event.
	 */
	bool idle;

	struct mpd_status *status;
	const struct mpd_song *song;

	int volume;
	unsigned update_id;

	/**
	 * A bit mask of idle events occurred since the last update.
	 */
	enum mpd_idle events;
};

enum {
	/**
	 * all idle events the version of libmpdclient, ncmpc is compiled
	 * against, supports
	 */
	MPD_IDLE_ALL = MPD_IDLE_DATABASE
		| MPD_IDLE_STORED_PLAYLIST
		| MPD_IDLE_QUEUE
		| MPD_IDLE_PLAYER
		| MPD_IDLE_MIXER
		| MPD_IDLE_OUTPUT
		| MPD_IDLE_OPTIONS
		| MPD_IDLE_UPDATE
#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
		| MPD_IDLE_STICKER
		| MPD_IDLE_SUBSCRIPTION
		| MPD_IDLE_MESSAGE
#endif
};

/** functions ***************************************************************/

bool
mpdclient_handle_error(struct mpdclient *c);

static inline bool
mpdclient_finish_command(struct mpdclient *c)
{
	return mpd_response_finish(c->connection)
		? true : mpdclient_handle_error(c);
}

struct mpdclient *
mpdclient_new(void);

void mpdclient_free(struct mpdclient *c);

G_GNUC_PURE
static inline bool
mpdclient_is_connected(const struct mpdclient *c)
{
	return c->connection != NULL;
}

G_GNUC_PURE
static inline bool
mpdclient_is_playing(const struct mpdclient *c)
{
	return c->status != NULL &&
		(mpd_status_get_state(c->status) == MPD_STATE_PLAY ||
		 mpd_status_get_state(c->status) == MPD_STATE_PAUSE);
}

G_GNUC_PURE
static inline const struct mpd_song *
mpdclient_get_current_song(const struct mpdclient *c)
{
	return c->song != NULL && mpdclient_is_playing(c)
		? c->song
		: NULL;
}

bool
mpdclient_connect(struct mpdclient *c, const gchar *host, gint port,
		  unsigned timeout_ms, const gchar *password);

void
mpdclient_disconnect(struct mpdclient *c);

bool
mpdclient_update(struct mpdclient *c);

struct mpd_connection *
mpdclient_get_connection(struct mpdclient *c);

void
mpdclient_put_connection(struct mpdclient *c);

/**
 * To be implemented by the application: mpdclient.c calls this to
 * display an error message.
 */
void
mpdclient_ui_error(const char *message);

/*** MPD Commands  **********************************************************/

bool
mpdclient_cmd_crop(struct mpdclient *c);

bool
mpdclient_cmd_clear(struct mpdclient *c);

bool
mpdclient_cmd_volume(struct mpdclient *c, gint value);

bool
mpdclient_cmd_volume_up(struct mpdclient *c);

bool
mpdclient_cmd_volume_down(struct mpdclient *c);

bool
mpdclient_cmd_add_path(struct mpdclient *c, const gchar *path);

bool
mpdclient_cmd_add(struct mpdclient *c, const struct mpd_song *song);

bool
mpdclient_cmd_delete(struct mpdclient *c, gint index);

bool
mpdclient_cmd_delete_range(struct mpdclient *c, unsigned start, unsigned end);

bool
mpdclient_cmd_move(struct mpdclient *c, unsigned dest, unsigned src);

#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
bool
mpdclient_cmd_subscribe(struct mpdclient *c, const char *channel);

bool
mpdclient_cmd_unsubscribe(struct mpdclient *c, const char *channel);

bool
mpdclient_cmd_send_message(struct mpdclient *c, const char *channel,
			   const char *text);

bool
mpdclient_send_read_messages(struct mpdclient *c);

struct mpd_message *
mpdclient_recv_message(struct mpdclient *c);
#endif

/*** playlist functions  **************************************************/

/* update the complete playlist */
bool
mpdclient_playlist_update(struct mpdclient *c);

/* get playlist changes */
bool
mpdclient_playlist_update_changes(struct mpdclient *c);

/* add all songs in filelist to the playlist */
bool
mpdclient_filelist_add_all(struct mpdclient *c, struct filelist *fl);

/* sort by list-format */
gint compare_filelistentry_format(gconstpointer filelist_entry1, gconstpointer filelist_entry2);

#endif
