/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "filelist.hxx"

#include <mpd/client.h>

#include <glib.h>

#include <algorithm>

#include <string.h>
#include <assert.h>

FileListEntry::~FileListEntry()
{
	if (entity)
		mpd_entity_free(entity);
}

gcc_pure
static bool
Less(const struct mpd_entity &a, struct mpd_entity &b)
{
	const auto a_type = mpd_entity_get_type(&a);
	const auto b_type = mpd_entity_get_type(&b);
	if (a_type != b_type)
		return a_type < b_type;

	switch (a_type) {
	case MPD_ENTITY_TYPE_UNKNOWN:
		break;

	case MPD_ENTITY_TYPE_DIRECTORY:
		return g_utf8_collate(mpd_directory_get_path(mpd_entity_get_directory(&a)),
				      mpd_directory_get_path(mpd_entity_get_directory(&b))) < 0;

	case MPD_ENTITY_TYPE_SONG:
		return false;

	case MPD_ENTITY_TYPE_PLAYLIST:
		return g_utf8_collate(mpd_playlist_get_path(mpd_entity_get_playlist(&a)),
				      mpd_playlist_get_path(mpd_entity_get_playlist(&b))) < 0;
	}

	return false;
}

gcc_pure
static bool
Less(const struct mpd_entity *a, struct mpd_entity *b)
{
	if (a == nullptr)
		return b != nullptr;
	else if (b == nullptr)
		return false;
	else
		return Less(*a, *b);
}

bool
FileListEntry::operator<(const FileListEntry &other) const
{
	return Less(entity, other.entity);
}

FileList::~FileList()
{
	for (auto *entry : entries)
		delete entry;
}

FileListEntry &
FileList::emplace_back(struct mpd_entity *entity)
{
	auto *entry = new FileListEntry(entity);
	entries.push_back(entry);
	return *entry;
}

void
FileList::MoveFrom(FileList &&src)
{
	entries.reserve(size() + src.size());
	entries.insert(entries.end(), src.entries.begin(), src.entries.end());
	src.entries.clear();
}

void
FileList::Sort()
{
	std::stable_sort(entries.begin(), entries.end(),
			 [](const FileListEntry *a, const FileListEntry *b){
				 return *a < *b;
			 });
}

void
FileList::RemoveDuplicateSongs()
{
	for (int i = size() - 1; i >= 0; --i) {
		auto &entry = (*this)[i];

		if (entry.entity == nullptr ||
		    mpd_entity_get_type(entry.entity) != MPD_ENTITY_TYPE_SONG)
			continue;

		const auto *song = mpd_entity_get_song(entry.entity);
		if (FindSong(*song) < i) {
			entries.erase(std::next(entries.begin(), i));
			delete &entry;
		}
	}
}

static bool
same_song(const struct mpd_song *a, const struct mpd_song *b)
{
	return strcmp(mpd_song_get_uri(a), mpd_song_get_uri(b)) == 0;
}

int
FileList::FindSong(const struct mpd_song &song) const
{
	for (unsigned i = 0; i < size(); ++i) {
		auto &entry = (*this)[i];
		const auto *entity  = entry.entity;

		if (entity != nullptr &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const auto *song2 = mpd_entity_get_song(entity);
			if (same_song(&song, song2))
				return i;
		}
	}

	return -1;
}

int
FileList::FindDirectory(const char *name) const
{
	assert(name != nullptr);

	for (unsigned i = 0; i < size(); ++i) {
		auto &entry = (*this)[i];
		const auto *entity = entry.entity;

		if (entity != nullptr &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY &&
		    strcmp(mpd_directory_get_path(mpd_entity_get_directory(entity)),
			   name) == 0)
			return i;
	}

	return -1;
}

void
FileList::Receive(struct mpd_connection &connection)
{
	struct mpd_entity *entity;

	while ((entity = mpd_recv_entity(&connection)) != nullptr)
		emplace_back(entity);
}

FileList *
filelist_new_recv(struct mpd_connection *connection)
{
	auto *filelist = new FileList();
	filelist->Receive(*connection);
	return filelist;
}
