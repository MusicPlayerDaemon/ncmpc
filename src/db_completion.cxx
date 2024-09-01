// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "db_completion.hxx"
#include "Completion.hxx"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "util/ScopeExit.hxx"

#include <string>

void
gcmp_list_from_path(struct mpdclient &c, const char *path,
		    Completion &completion,
		    int types)
{
	auto *connection = c.GetConnection();
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
