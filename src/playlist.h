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

#include <glib.h>

struct mpdclient;

typedef struct mpdclient_playlist {
	/* playlist id */
	long long id;

	/* true if the list is updated */
	gboolean updated;

	/* the list */
	GArray *list;
} mpdclient_playlist_t;

/* free a playlist */
gint mpdclient_playlist_free(mpdclient_playlist_t *playlist);

struct mpd_song *playlist_lookup_song(struct mpdclient *c, gint id);

struct mpd_song *playlist_get_song(struct mpdclient *c, gint index);

gint playlist_get_index(struct mpdclient *c, struct mpd_song *song);

gint playlist_get_index_from_id(struct mpdclient *c, gint id);

gint playlist_get_index_from_file(struct mpdclient *c, gchar *filename);

#endif
