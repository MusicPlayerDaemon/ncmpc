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

#include <string.h>
#include <assert.h>

filelist::~filelist()
{
	for (unsigned i = 0; i < size(); ++i) {
		struct filelist_entry *entry = (*this)[i];

		if (entry->entity)
			mpd_entity_free(entry->entity);

		g_slice_free(struct filelist_entry, entry);
	}

	g_ptr_array_free(entries, true);
}

struct filelist_entry *
filelist::emplace_back(struct mpd_entity *entity)
{
	struct filelist_entry *entry = g_slice_new(struct filelist_entry);

	entry->flags = 0;
	entry->entity = entity;

	g_ptr_array_add(entries, entry);

	return entry;
}

void
filelist::MoveFrom(struct filelist &&src)
{
	for (unsigned i = 0; i < src.size(); ++i)
		g_ptr_array_add(entries, g_ptr_array_index(src.entries, i));

	g_ptr_array_set_size(src.entries, 0);
}

static gint
filelist_compare_indirect(gconstpointer ap, gconstpointer bp, gpointer data)
{
	GCompareFunc compare_func = (GCompareFunc)data;
	gconstpointer a = *(const gconstpointer*)ap;
	gconstpointer b = *(const gconstpointer*)bp;

	return compare_func(a, b);
}

gint
compare_filelist_entry_path(gconstpointer filelist_entry1,
			    gconstpointer filelist_entry2)
{
	const struct mpd_entity *e1, *e2;

	e1 = ((const struct filelist_entry *)filelist_entry1)->entity;
	e2 = ((const struct filelist_entry *)filelist_entry2)->entity;

	int n = 0;
	if (e1 != nullptr && e2 != nullptr &&
	    mpd_entity_get_type(e1) == mpd_entity_get_type(e2)) {
		switch (mpd_entity_get_type(e1)) {
		case MPD_ENTITY_TYPE_UNKNOWN:
			break;
		case MPD_ENTITY_TYPE_DIRECTORY:
			n = g_utf8_collate(mpd_directory_get_path(mpd_entity_get_directory(e1)),
					   mpd_directory_get_path(mpd_entity_get_directory(e2)));
			break;
		case MPD_ENTITY_TYPE_SONG:
			break;
		case MPD_ENTITY_TYPE_PLAYLIST:
			n = g_utf8_collate(mpd_playlist_get_path(mpd_entity_get_playlist(e1)),
					   mpd_playlist_get_path(mpd_entity_get_playlist(e2)));
		}
	}
	return n;
}

/* Sorts the whole filelist, at the moment used by filelist_search */
void
filelist::SortAll(GCompareFunc compare_func)
{
	g_ptr_array_sort_with_data(entries,
				   filelist_compare_indirect,
				   (gpointer)compare_func);
}


/* Only sorts the directories and playlist files.
 * The songs stay in the order it came from mpd. */
void
filelist::SortDirectoriesPlaylists(GCompareFunc compare_func)
{
	const struct mpd_entity *iter;

	if (entries->len < 2)
		return;

	/* If the first entry is nullptr, skip it, because nullptr stands for "[..]" */
	iter = ((struct filelist_entry*) g_ptr_array_index(entries, 0))->entity;
	unsigned first = iter == nullptr ? 1 : 0, last;

	/* find the last directory entry */
	for (last = first+1; last < entries->len; last++) {
		iter = ((struct filelist_entry*) g_ptr_array_index(entries, last))->entity;
		if (mpd_entity_get_type(iter) != MPD_ENTITY_TYPE_DIRECTORY)
			break;
	}
	if (last == entries->len - 1)
		last++;
	/* sort the directories */
	if (last - first > 1)
		g_qsort_with_data(entries->pdata + first,
				  last - first, sizeof(gpointer),
				  filelist_compare_indirect, (gpointer)compare_func);
	/* find the first playlist entry */
	for (first = last; first < entries->len; first++) {
		iter = ((struct filelist_entry*) g_ptr_array_index(entries, first))->entity;
		if (mpd_entity_get_type(iter) == MPD_ENTITY_TYPE_PLAYLIST)
			break;
	}
	/* sort the playlist entries */
	if (entries->len - first > 1)
		g_qsort_with_data(entries->pdata + first,
				  entries->len - first, sizeof(gpointer),
				  filelist_compare_indirect,
				  (gpointer)compare_func);
}

void
filelist::RemoveDuplicateSongs()
{
	for (int i = size() - 1; i >= 0; --i) {
		struct filelist_entry *entry = (*this)[i];
		const struct mpd_song *song;

		if (entry->entity == nullptr ||
		    mpd_entity_get_type(entry->entity) != MPD_ENTITY_TYPE_SONG)
			continue;

		song = mpd_entity_get_song(entry->entity);
		if (FindSong(*song) < i) {
			g_ptr_array_remove_index(entries, i);
			mpd_entity_free(entry->entity);
			g_slice_free(struct filelist_entry, entry);
		}
	}
}

static bool
same_song(const struct mpd_song *a, const struct mpd_song *b)
{
	return strcmp(mpd_song_get_uri(a), mpd_song_get_uri(b)) == 0;
}

int
filelist::FindSong(const struct mpd_song &song) const
{
	for (unsigned i = 0; i < size(); ++i) {
		struct filelist_entry *entry = (*this)[i];
		const struct mpd_entity *entity  = entry->entity;

		if (entity != nullptr &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const struct mpd_song *song2 =
				mpd_entity_get_song(entity);

			if (same_song(&song, song2))
				return i;
		}
	}

	return -1;
}

int
filelist::FindDirectory(const char *name) const
{
	assert(name != nullptr);

	for (unsigned i = 0; i < size(); ++i) {
		struct filelist_entry *entry = (*this)[i];
		const struct mpd_entity *entity  = entry->entity;

		if (entity != nullptr &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY &&
		    strcmp(mpd_directory_get_path(mpd_entity_get_directory(entity)),
			   name) == 0)
			return i;
	}

	return -1;
}

void
filelist::Receive(struct mpd_connection &connection)
{
	struct mpd_entity *entity;

	while ((entity = mpd_recv_entity(&connection)) != nullptr)
		emplace_back(entity);
}

struct filelist *
filelist_new_recv(struct mpd_connection *connection)
{
	auto *filelist = new struct filelist();
	filelist->Receive(*connection);
	return filelist;
}
