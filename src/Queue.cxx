// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Queue.hxx"
#include "util/StringAPI.hxx"

#include <algorithm>

void
MpdQueue::clear() noexcept
{
	version = 0;
	items.clear();
}

const struct mpd_song *
MpdQueue::GetChecked(int idx) const noexcept
{
	if (idx < 0 || (size_type)idx >= size())
		return nullptr;

	return &(*this)[idx];
}

void
MpdQueue::Move(unsigned dest, unsigned src) noexcept
{
	assert(src < size());
	assert(dest < size());
	assert(src != dest);

	auto song = std::move(items[src]);

	if (src < dest) {
		std::move(std::next(items.begin(), src + 1),
			  std::next(items.begin(), dest + 1),
			  std::next(items.begin(), src));
	} else {
		std::move_backward(std::next(items.begin(), dest),
				   std::next(items.begin(), src),
				   std::next(items.begin(), src + 1));
	}

	assert(!items[dest]);
	items[dest] = std::move(song);
}

MpdQueue::size_type
MpdQueue::FindByReference(const struct mpd_song &song) const noexcept
{
	for (size_type i = 0;; ++i) {
		assert(i < size());

		if (&(*this)[i] == &song)
			return i;
	}
}

int
MpdQueue::FindById(unsigned id) const noexcept
{
	for (size_type i = 0; i < size(); ++i) {
		const auto &song = (*this)[i];
		if (mpd_song_get_id(&song) == id)
			return i;
	}

	return -1;
}

int
MpdQueue::FindByUri(const char *filename) const noexcept
{
	for (size_type i = 0; i < size(); ++i) {
		const auto &song = (*this)[i];
		if (StringIsEqual(mpd_song_get_uri(&song), filename))
			return i;
	}

	return -1;
}
