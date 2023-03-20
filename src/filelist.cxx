// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "filelist.hxx"
#include "util/StringUTF8.hxx"

#include <mpd/client.h>

#include <algorithm>

#include <string.h>
#include <assert.h>

FileListEntry::~FileListEntry() noexcept
{
	if (entity)
		mpd_entity_free(entity);
}

[[gnu::pure]]
static bool
Less(const struct mpd_entity &a, struct mpd_entity &b) noexcept
{
	const auto a_type = mpd_entity_get_type(&a);
	const auto b_type = mpd_entity_get_type(&b);
	if (a_type != b_type)
		return a_type < b_type;

	switch (a_type) {
	case MPD_ENTITY_TYPE_UNKNOWN:
		break;

	case MPD_ENTITY_TYPE_DIRECTORY:
		return CollateUTF8(mpd_directory_get_path(mpd_entity_get_directory(&a)),
				   mpd_directory_get_path(mpd_entity_get_directory(&b))) < 0;

	case MPD_ENTITY_TYPE_SONG:
		return false;

	case MPD_ENTITY_TYPE_PLAYLIST:
		return CollateUTF8(mpd_playlist_get_path(mpd_entity_get_playlist(&a)),
				   mpd_playlist_get_path(mpd_entity_get_playlist(&b))) < 0;
	}

	return false;
}

[[gnu::pure]]
static bool
Less(const struct mpd_entity *a, struct mpd_entity *b) noexcept
{
	if (a == nullptr)
		return b != nullptr;
	else if (b == nullptr)
		return false;
	else
		return Less(*a, *b);
}

bool
FileListEntry::operator<(const FileListEntry &other) const noexcept
{
	return Less(entity, other.entity);
}

FileListEntry &
FileList::emplace_back(struct mpd_entity *entity) noexcept
{
	entries.emplace_back(entity);
	return entries.back();
}

void
FileList::MoveFrom(FileList &&src) noexcept
{
	entries.reserve(size() + src.size());
	for (auto &i : src.entries)
		entries.emplace_back(std::move(i));
	src.entries.clear();
}

void
FileList::Sort() noexcept
{
	std::stable_sort(entries.begin(), entries.end());
}

void
FileList::RemoveDuplicateSongs() noexcept
{
	for (int i = size() - 1; i >= 0; --i) {
		auto &entry = (*this)[i];

		if (entry.entity == nullptr ||
		    mpd_entity_get_type(entry.entity) != MPD_ENTITY_TYPE_SONG)
			continue;

		const auto *song = mpd_entity_get_song(entry.entity);
		if (FindSong(*song) < i)
			entries.erase(std::next(entries.begin(), i));
	}
}

[[gnu::pure]]
static bool
same_song(const struct mpd_song *a, const struct mpd_song *b) noexcept
{
	return strcmp(mpd_song_get_uri(a), mpd_song_get_uri(b)) == 0;
}

int
FileList::FindSong(const struct mpd_song &song) const noexcept
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
FileList::FindDirectory(const char *name) const noexcept
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
FileList::Receive(struct mpd_connection &connection) noexcept
{
	struct mpd_entity *entity;

	while ((entity = mpd_recv_entity(&connection)) != nullptr)
		emplace_back(entity);
}

std::unique_ptr<FileList>
filelist_new_recv(struct mpd_connection *connection) noexcept
{
	auto filelist = std::make_unique<FileList>();
	filelist->Receive(*connection);
	return filelist;
}
