/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "util/Compiler.h"

#include <memory>
#include <vector>
#include <utility>

struct mpd_connection;
struct mpd_song;

struct FileListEntry {
	unsigned flags = 0;
	struct mpd_entity *entity;

	explicit FileListEntry(struct mpd_entity *_entity) noexcept
		:entity(_entity) {}
	~FileListEntry() noexcept;

	FileListEntry(FileListEntry &&src) noexcept
		:flags(src.flags),
		 entity(std::exchange(src.entity, nullptr)) {}

	FileListEntry &operator=(FileListEntry &&src) noexcept {
		using std::swap;
		flags = src.flags;
		swap(entity, src.entity);
		return *this;
	}

	gcc_pure
	bool operator<(const FileListEntry &other) const noexcept;
};

class FileList {
	using Vector = std::vector<FileListEntry>;

	/* the list */
	Vector entries;

public:
	using size_type = Vector::size_type;

	FileList() = default;

	FileList(const FileList &) = delete;
	FileList &operator=(const FileList &) = delete;

	size_type size() const noexcept {
		return entries.size();
	}

	bool empty() const noexcept {
		return entries.empty();
	}

	FileListEntry &operator[](size_type i) noexcept {
		return entries[i];
	}

	const FileListEntry &operator[](size_type i) const noexcept {
		return entries[i];
	}

	FileListEntry &emplace_back(struct mpd_entity *entity) noexcept;

	void MoveFrom(FileList &&src) noexcept;

	/**
	 * Sort the whole list.
	 */
	void Sort() noexcept;

	/**
	 * Eliminates duplicate songs from the FileList.
	 */
	void RemoveDuplicateSongs() noexcept;

	gcc_pure
	int FindSong(const struct mpd_song &song) const noexcept;

	gcc_pure
	int FindDirectory(const char *name) const noexcept;

	/**
	 * Receives entities from the connection, and appends them to the
	 * specified FileList.  This does not finish the response, and does
	 * not check for errors.
	 */
	void Receive(struct mpd_connection &connection) noexcept;
};

/**
 * Creates a new FileList and receives entities from the connection.
 * This does not finish the response, and does not check for errors.
 */
std::unique_ptr<FileList>
filelist_new_recv(struct mpd_connection *connection) noexcept;

#endif
