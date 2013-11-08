/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#include "filelist.h"

#include <mpd/client.h>

#include <stdbool.h>
#include <string.h>
#include <assert.h>

struct filelist *
filelist_new(void)
{
	struct filelist *filelist = g_malloc(sizeof(*filelist));

	filelist->entries = g_ptr_array_new();

	return filelist;
}

void
filelist_free(struct filelist *filelist)
{
	for (unsigned i = 0; i < filelist_length(filelist); ++i) {
		struct filelist_entry *entry = filelist_get(filelist, i);

		if (entry->entity)
			mpd_entity_free(entry->entity);

		g_slice_free(struct filelist_entry, entry);
	}

	g_ptr_array_free(filelist->entries, TRUE);
	g_free(filelist);
}

struct filelist_entry *
filelist_append(struct filelist *filelist, struct mpd_entity *entity)
{
	struct filelist_entry *entry = g_slice_new(struct filelist_entry);

	entry->flags = 0;
	entry->entity = entity;

	g_ptr_array_add(filelist->entries, entry);

	return entry;
}

void
filelist_move(struct filelist *filelist, struct filelist *from)
{
	for (unsigned i = 0; i < filelist_length(from); ++i)
		g_ptr_array_add(filelist->entries,
				g_ptr_array_index(from->entries, i));

	g_ptr_array_set_size(from->entries, 0);
}

static gint
filelist_compare_indirect(gconstpointer ap, gconstpointer bp, gpointer data)
{
	GCompareFunc compare_func = data;
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
	if (e1 != NULL && e2 != NULL &&
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
filelist_sort_all(struct filelist *filelist, GCompareFunc compare_func)
{
	g_ptr_array_sort_with_data(filelist->entries,
			filelist_compare_indirect,
			compare_func);
}


/* Only sorts the directories and playlist files.
 * The songs stay in the order it came from mpd. */
void
filelist_sort_dir_play(struct filelist *filelist, GCompareFunc compare_func)
{
	const struct mpd_entity *iter;

	assert(filelist && filelist->entries);

	if (filelist->entries->len < 2)
		return;

	/* If the first entry is NULL, skip it, because NULL stands for "[..]" */
	iter = ((struct filelist_entry*) g_ptr_array_index(filelist->entries, 0))->entity;
	unsigned first = iter == NULL ? 1 : 0, last;

	/* find the last directory entry */
	for (last = first+1; last < filelist->entries->len; last++) {
		iter = ((struct filelist_entry*) g_ptr_array_index(filelist->entries, last))->entity;
		if (mpd_entity_get_type(iter) != MPD_ENTITY_TYPE_DIRECTORY)
			break;
	}
	if (last == filelist->entries->len - 1)
		last++;
	/* sort the directories */
	if (last - first > 1)
		g_qsort_with_data(filelist->entries->pdata + first,
				last - first, sizeof(gpointer),
				filelist_compare_indirect, compare_func);
	/* find the first playlist entry */
	for (first = last; first < filelist->entries->len; first++) {
		iter = ((struct filelist_entry*) g_ptr_array_index(filelist->entries, first))->entity;
		if (mpd_entity_get_type(iter) == MPD_ENTITY_TYPE_PLAYLIST)
			break;
	}
	/* sort the playlist entries */
	if (filelist->entries->len - first > 1)
		g_qsort_with_data(filelist->entries->pdata + first,
				filelist->entries->len - first, sizeof(gpointer),
				filelist_compare_indirect, compare_func);
}

void
filelist_no_duplicates(struct filelist *filelist)
{
	for (int i = filelist_length(filelist) - 1; i >= 0; --i) {
		struct filelist_entry *entry = filelist_get(filelist, i);
		const struct mpd_song *song;

		if (entry->entity == NULL ||
		    mpd_entity_get_type(entry->entity) != MPD_ENTITY_TYPE_SONG)
			continue;

		song = mpd_entity_get_song(entry->entity);
		if (filelist_find_song(filelist, song) < i) {
			g_ptr_array_remove_index(filelist->entries, i);
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
filelist_find_song(const struct filelist *fl, const struct mpd_song *song)
{
	assert(song != NULL);

	for (unsigned i = 0; i < filelist_length(fl); ++i) {
		struct filelist_entry *entry = filelist_get(fl, i);
		const struct mpd_entity *entity  = entry->entity;

		if (entity != NULL &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const struct mpd_song *song2 =
				mpd_entity_get_song(entity);

			if (same_song(song, song2))
				return i;
		}
	}

	return -1;
}

int
filelist_find_directory(const struct filelist *filelist, const char *name)
{
	assert(name != NULL);

	for (unsigned i = 0; i < filelist_length(filelist); ++i) {
		struct filelist_entry *entry = filelist_get(filelist, i);
		const struct mpd_entity *entity  = entry->entity;

		if (entity != NULL &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY &&
		    strcmp(mpd_directory_get_path(mpd_entity_get_directory(entity)),
			   name) == 0)
			return i;
	}

	return -1;
}

void
filelist_recv(struct filelist *filelist, struct mpd_connection *connection)
{
	struct mpd_entity *entity;

	while ((entity = mpd_recv_entity(connection)) != NULL)
		filelist_append(filelist, entity);
}

struct filelist *
filelist_new_recv(struct mpd_connection *connection)
{
	struct filelist *filelist = filelist_new();
	filelist_recv(filelist, connection);
	return filelist;
}
