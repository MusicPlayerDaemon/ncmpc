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

#ifndef FILELIST_H
#define FILELIST_H

#include "Compiler.h"

#include <glib.h>

struct mpd_connection;
struct mpd_song;

struct FileListEntry {
	guint flags;
	struct mpd_entity *entity;
};

struct FileList {
	/* the list */
	GPtrArray *entries;

	FileList()
		:entries(g_ptr_array_new()) {}

	~FileList();

	FileList(const FileList &) = delete;
	FileList &operator=(const FileList &) = delete;

	guint size() const {
		return entries->len;
	}

	bool empty() const {
		return size() == 0;
	}

	FileListEntry *operator[](guint i) const {
		return (FileListEntry *)g_ptr_array_index(entries, i);
	}

	FileListEntry *emplace_back(struct mpd_entity *entity);

	void MoveFrom(FileList &&src);

	/**
	 * Sort the whole list.
	 */
	void SortAll(GCompareFunc compare_func);

	/**
	 * Only sort the directories and playlist files.
	 * The songs stay in the order it came from MPD.
	 */
	void SortDirectoriesPlaylists(GCompareFunc compare_func);

	/**
	 * Eliminates duplicate songs from the FileList.
	 */
	void RemoveDuplicateSongs();

	gcc_pure
	int FindSong(const struct mpd_song &song) const;

	gcc_pure
	int FindDirectory(const char *name) const;

	/**
	 * Receives entities from the connection, and appends them to the
	 * specified FileList.  This does not finish the response, and does
	 * not check for errors.
	 */
	void Receive(struct mpd_connection &connection);
};

gcc_pure
gint
compare_filelist_entry_path(gconstpointer filelist_entry1,
			    gconstpointer filelist_entry2);

/**
 * Creates a new FileList and receives entities from the connection.
 * This does not finish the response, and does not check for errors.
 */
FileList *
filelist_new_recv(struct mpd_connection *connection);

#endif
