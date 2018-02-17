/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "db_completion.hxx"
#include "charset.hxx"
#include "mpdclient.hxx"

GList *
gcmp_list_from_path(struct mpdclient *c, const gchar *path,
		    GList *list, gint types)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return list;

	mpd_send_list_meta(connection, path);

	struct mpd_entity *entity;
	while ((entity = mpd_recv_entity(connection)) != nullptr) {
		char *name;

		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY &&
		    types & GCMP_TYPE_DIR) {
			const struct mpd_directory *dir =
				mpd_entity_get_directory(entity);
			gchar *tmp = utf8_to_locale(mpd_directory_get_path(dir));
			name = g_strconcat(tmp, "/", nullptr);
			g_free(tmp);
		} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG &&
			   types & GCMP_TYPE_FILE) {
			const struct mpd_song *song =
				mpd_entity_get_song(entity);
			name = utf8_to_locale(mpd_song_get_uri(song));
		} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST &&
			   types & GCMP_TYPE_PLAYLIST) {
			const struct mpd_playlist *playlist =
				mpd_entity_get_playlist(entity);
			name = utf8_to_locale(mpd_playlist_get_path(playlist));
		} else {
			mpd_entity_free(entity);
			continue;
		}

		list = g_list_append(list, name);
		mpd_entity_free(entity);
	}

	return list;
}
