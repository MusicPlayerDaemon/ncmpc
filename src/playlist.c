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

#include "playlist.h"
#include "mpdclient.h"

#include <string.h>

#define ENABLE_PLCHANGES

#define MPD_ERROR(c) (c==NULL || c->connection==NULL || c->connection->error)

void
playlist_init(struct mpdclient_playlist *playlist)
{
	playlist->id = 0;
	playlist->list = g_ptr_array_sized_new(1024);
}

void
playlist_clear(struct mpdclient_playlist *playlist)
{
	guint i;

	playlist->id = 0;

	for (i = 0; i < playlist->list->len; ++i) {
		struct mpd_song *song = playlist_get(playlist, i);

		mpd_song_free(song);
	}

	g_ptr_array_set_size(playlist->list, 0);
}

gint
mpdclient_playlist_free(struct mpdclient_playlist *playlist)
{
	if (playlist->list != NULL) {
		playlist_clear(playlist);
		g_ptr_array_free(playlist->list, TRUE);
	}

	memset(playlist, 0, sizeof(*playlist));
	return 0;
}

struct mpd_song *
playlist_get_song(struct mpdclient *c, gint idx)
{
	if (idx < 0 || (guint)idx >= c->playlist.list->len)
		return NULL;

	return playlist_get(&c->playlist, idx);
}

struct mpd_song *
playlist_lookup_song(struct mpdclient *c, unsigned id)
{
	guint i;

	for (i = 0; i < c->playlist.list->len; ++i) {
		struct mpd_song *song = playlist_get(&c->playlist, i);
		if (mpd_song_get_id(song) == id)
			return song;
	}

	return NULL;
}

gint
playlist_get_index(const struct mpdclient *c, const struct mpd_song *song)
{
	guint i;

	for (i = 0; i < c->playlist.list->len; ++i) {
		if (playlist_get(&c->playlist, i) == song)
			return (gint)i;
	}

	return -1;
}

gint
playlist_get_index_from_id(const struct mpdclient *c, unsigned id)
{
	guint i;

	for (i = 0; i < c->playlist.list->len; ++i) {
		const struct mpd_song *song = playlist_get(&c->playlist, i);
		if (mpd_song_get_id(song) == id)
			return (gint)i;
	}

	return -1;
}

gint
playlist_get_index_from_file(const struct mpdclient *c, const gchar *filename)
{
	guint i;

	for (i = 0; i < c->playlist.list->len; ++i) {
		struct mpd_song *song = playlist_get(&c->playlist, i);
		const char *uri = mpd_song_get_uri(song);

		if (uri != NULL && strcmp(uri, filename) == 0)
			return (gint)i;
	}

	return -1;
}
