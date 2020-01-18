/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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
#include "Completion.hxx"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "util/ScopeExit.hxx"

#include <string>

void
gcmp_list_from_path(struct mpdclient *c, const char *path,
		    Completion &completion,
		    int types)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return;

	mpd_send_list_meta(connection, path);

	struct mpd_entity *entity;
	while ((entity = mpd_recv_entity(connection)) != nullptr) {
		AtScopeExit(entity) { mpd_entity_free(entity); };

		std::string name;
		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY &&
		    types & GCMP_TYPE_DIR) {
			const struct mpd_directory *dir =
				mpd_entity_get_directory(entity);
			name = Utf8ToLocale(mpd_directory_get_path(dir)).c_str();
			name.push_back('/');
		} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG &&
			   types & GCMP_TYPE_FILE) {
			const struct mpd_song *song =
				mpd_entity_get_song(entity);
			name = Utf8ToLocale(mpd_song_get_uri(song)).c_str();
		} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST &&
			   types & GCMP_TYPE_PLAYLIST) {
			const struct mpd_playlist *playlist =
				mpd_entity_get_playlist(entity);
			name = Utf8ToLocale(mpd_playlist_get_path(playlist)).c_str();
		} else {
			continue;
		}

		completion.emplace(std::move(name));
	}
}
