#ifndef MPDCLIENT_H
#define MPDCLIENT_H

#include "config.h"
#include "playlist.h"
#include "Compiler.h"

#include <mpd/client.h>

struct filelist;

struct mpdclient {
#ifdef ENABLE_ASYNC_CONNECT
	/**
	 * These settings are used to connect to MPD asynchronously.
	 */
	struct mpd_settings *settings;

#ifndef WIN32
	/**
	 * A second set of settings, just in case #settings did not
	 * work.  This is only used if #settings refers to a local
	 * socket path, and this one is supposed to be a fallback to
	 * IP on the default port (6600).
	 */
	struct mpd_settings *settings2;
#endif

#else
	const char *host;
	unsigned port;
#endif

	unsigned timeout_ms;

	const char *password;

	/* playlist */
	struct mpdclient_playlist playlist;

#ifdef ENABLE_ASYNC_CONNECT
	struct aconnect *async_connect;
#endif

	struct mpd_connection *connection;

	/**
	 * Tracks idle events.  It is automatically called by
	 * mpdclient_get_connection().
	 */
	struct mpd_glib_source *source;

	struct mpd_status *status;
	const struct mpd_song *song;

	/**
	 * The GLib source id which re-enters MPD idle mode before the
	 * next main loop interation.
	 */
	unsigned enter_idle_source_id;

	/**
	 * This attribute is incremented whenever the connection changes
	 * (i.e. on disconnection and (re-)connection).
	 */
	unsigned connection_id;

	int volume;

	/**
	 * A bit mask of idle events occurred since the last update.
	 */
	enum mpd_idle events;

#if defined(ENABLE_ASYNC_CONNECT) && !defined(WIN32)
	bool connecting2;
#endif

	/**
	 * This attribute is true when the connection is currently in
	 * "idle" mode, and the #mpd_glib_source waits for an event.
	 */
	bool idle;

	/**
	 * Is MPD currently playing?
	 */
	bool playing;
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
		| MPD_IDLE_STICKER
		| MPD_IDLE_SUBSCRIPTION
		| MPD_IDLE_MESSAGE
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
mpdclient_new(const gchar *host, unsigned port,
	      unsigned timeout_ms, const gchar *password);

void mpdclient_free(struct mpdclient *c);

/**
 * Determine a human-readable "name" of the settings currently used to
 * connect to MPD.
 *
 * @return an allocated string that needs to be freed (with g_free())
 * by the caller
 */
char *
mpdclient_settings_name(const struct mpdclient *c);

gcc_pure
static inline bool
mpdclient_is_connected(const struct mpdclient *c)
{
	return c->connection != NULL;
}

/**
 * Is this object "dead"?  i.e. not connected and not currently doing
 * anything to connect.
 */
gcc_pure
static inline bool
mpdclient_is_dead(const struct mpdclient *c)
{
	return c->connection == NULL
#ifdef ENABLE_ASYNC_CONNECT
		&& c->async_connect == NULL
#endif
		;
}

gcc_pure
static inline bool
mpdclient_is_playing(const struct mpdclient *c)
{
	return c->status != NULL &&
		(mpd_status_get_state(c->status) == MPD_STATE_PLAY ||
		 mpd_status_get_state(c->status) == MPD_STATE_PAUSE);
}

gcc_pure
static inline const struct mpd_song *
mpdclient_get_current_song(const struct mpdclient *c)
{
	return c->song != NULL && mpdclient_is_playing(c)
		? c->song
		: NULL;
}

void
mpdclient_connect(struct mpdclient *c);

void
mpdclient_disconnect(struct mpdclient *c);

bool
mpdclient_update(struct mpdclient *c);

struct mpd_connection *
mpdclient_get_connection(struct mpdclient *c);

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

#endif
