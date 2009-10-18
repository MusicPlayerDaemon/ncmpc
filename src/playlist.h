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

#ifndef MPDCLIENT_PLAYLIST_H
#define MPDCLIENT_PLAYLIST_H

#include <mpd/client.h>

#include <assert.h>
#include <glib.h>

struct mpdclient_playlist {
	/* queue version number (obtained from mpd_status) */
	unsigned version;

	/* the list */
	GPtrArray *list;
};

void
playlist_init(struct mpdclient_playlist *playlist);

/** remove and free all songs in the playlist */
void
playlist_clear(struct mpdclient_playlist *playlist);

/* free a playlist */
gint
mpdclient_playlist_free(struct mpdclient_playlist *playlist);

static inline guint
playlist_length(const struct mpdclient_playlist *playlist)
{
	assert(playlist != NULL);
	assert(playlist->list != NULL);

	return playlist->list->len;
}

static inline gboolean
playlist_is_empty(const struct mpdclient_playlist *playlist)
{
	return playlist_length(playlist) == 0;
}

static inline struct mpd_song *
playlist_get(const struct mpdclient_playlist *playlist, guint idx)
{
	assert(idx < playlist_length(playlist));

	return g_ptr_array_index(playlist->list, idx);
}

static inline void
playlist_append(struct mpdclient_playlist *playlist, const struct mpd_song *song)
{
	g_ptr_array_add(playlist->list, mpd_song_dup(song));
}

static inline void
playlist_set(const struct mpdclient_playlist *playlist, guint idx,
	     const struct mpd_song *song)
{
	assert(idx < playlist_length(playlist));

	g_ptr_array_index(playlist->list, idx) = mpd_song_dup(song);
}

static inline void
playlist_replace(struct mpdclient_playlist *playlist, guint idx,
		 const struct mpd_song *song)
{
	mpd_song_free(playlist_get(playlist, idx));
	playlist_set(playlist, idx, song);
}

static inline struct mpd_song *
playlist_remove_reuse(struct mpdclient_playlist *playlist, guint idx)
{
	return g_ptr_array_remove_index(playlist->list, idx);
}

static inline void
playlist_remove(struct mpdclient_playlist *playlist, guint idx)
{
	mpd_song_free(playlist_remove_reuse(playlist, idx));
}

void
playlist_move(struct mpdclient_playlist *playlist,
	      unsigned dest, unsigned src);

const struct mpd_song *
playlist_lookup_song(const struct mpdclient_playlist *playlist, unsigned id);

const struct mpd_song *
playlist_get_song(const struct mpdclient_playlist *playlist, gint index);

gint
playlist_get_index(const struct mpdclient_playlist *playlist,
		   const struct mpd_song *song);

gint
playlist_get_index_from_id(const struct mpdclient_playlist *playlist,
			   unsigned id);

gint
playlist_get_index_from_file(const struct mpdclient_playlist *playlist,
			     const gchar *filename);

static inline gint
playlist_get_index_from_same_song(const struct mpdclient_playlist *playlist,
				  const struct mpd_song *song)
{
	return playlist_get_index_from_file(playlist, mpd_song_get_uri(song));
}

gint
playlist_get_id_from_uri(const struct mpdclient_playlist *playlist,
			 const gchar *uri);

static inline gint
playlist_get_id_from_same_song(const struct mpdclient_playlist *playlist,
			       const struct mpd_song *song)
{
	return playlist_get_id_from_uri(playlist, mpd_song_get_uri(song));
}

#endif
