/*
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
 * (c) 2008 Max Kellermann <max@duempel.org>
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

#ifndef MPDCLIENT_PLAYLIST_H
#define MPDCLIENT_PLAYLIST_H

#include "libmpdclient.h"

#include <assert.h>
#include <glib.h>

struct mpdclient;

typedef struct mpdclient_playlist {
	/* playlist id */
	long long id;

	/* true if the list is updated */
	gboolean updated;

	/* the list */
	GPtrArray *list;
} mpdclient_playlist_t;

void
playlist_init(struct mpdclient_playlist *playlist);

/** remove and free all songs in the playlist */
void
playlist_clear(struct mpdclient_playlist *playlist);

/* free a playlist */
gint mpdclient_playlist_free(mpdclient_playlist_t *playlist);

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
playlist_append(struct mpdclient_playlist *playlist, const mpd_Song *song)
{
	g_ptr_array_add(playlist->list, mpd_songDup(song));
}

static inline void
playlist_set(const struct mpdclient_playlist *playlist, guint idx,
	     const mpd_Song *song)
{
	assert(idx < playlist_length(playlist));

	g_ptr_array_index(playlist->list, idx) = mpd_songDup(song);
}

static inline void
playlist_replace(struct mpdclient_playlist *playlist, guint idx,
		 const mpd_Song *song)
{
	mpd_freeSong(playlist_get(playlist, idx));
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
	mpd_freeSong(playlist_remove_reuse(playlist, idx));
}

static inline void
playlist_swap(struct mpdclient_playlist *playlist, guint idx1, guint idx2)
{
	mpd_Song *song1 = playlist_get(playlist, idx1);
	mpd_Song *song2 = playlist_get(playlist, idx2);
	gint n;

	/* update the songs position field */
	n = song1->pos;
	song1->pos = song2->pos;
	song2->pos = n;

	/* update the array */
	g_ptr_array_index(playlist->list, idx1) = song2;
	g_ptr_array_index(playlist->list, idx2) = song1;
}

struct mpd_song *playlist_lookup_song(struct mpdclient *c, gint id);

struct mpd_song *playlist_get_song(struct mpdclient *c, gint index);

gint playlist_get_index(struct mpdclient *c, struct mpd_song *song);

gint playlist_get_index_from_id(struct mpdclient *c, gint id);

gint playlist_get_index_from_file(struct mpdclient *c, gchar *filename);

#endif
