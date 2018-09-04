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

#include "Queue.hxx"

#include <algorithm>

#include <string.h>

void
MpdQueue::clear()
{
	version = 0;
	items.clear();
}

const struct mpd_song *
MpdQueue::GetChecked(int idx) const
{
	if (idx < 0 || (size_type)idx >= size())
		return nullptr;

	return &(*this)[idx];
}

void
MpdQueue::Move(unsigned dest, unsigned src)
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
		std::move(std::next(items.begin(), dest),
				   std::next(items.begin(), src),
				   std::next(items.begin(), dest + 1));
	}

	assert(!items[dest]);
	items[dest] = std::move(song);
}

MpdQueue::size_type
MpdQueue::FindByReference(const struct mpd_song &song) const
{
	for (size_type i = 0;; ++i) {
		assert(i < size());

		if (&(*this)[i] == &song)
			return i;
	}
}

int
MpdQueue::FindById(unsigned id) const
{
	for (size_type i = 0; i < size(); ++i) {
		const auto &song = (*this)[i];
		if (mpd_song_get_id(&song) == id)
			return i;
	}

	return -1;
}

int
MpdQueue::FindByUri(const char *filename) const
{
	for (size_type i = 0; i < size(); ++i) {
		const auto &song = (*this)[i];
		if (strcmp(mpd_song_get_uri(&song), filename) == 0)
			return i;
	}

	return -1;
}
