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

#include <string.h>

void
MpdQueue::clear()
{
	version = 0;

	for (unsigned i = 0; i < list->len; ++i) {
		auto *song = &(*this)[i];
		mpd_song_free(song);
	}

	g_ptr_array_set_size(list, 0);
}

MpdQueue::~MpdQueue()
{
	if (list != nullptr) {
		clear();
		g_ptr_array_free(list, true);
	}
}

const struct mpd_song *
MpdQueue::GetChecked(int idx) const
{
	if (idx < 0 || (guint)idx >= size())
		return nullptr;

	return &(*this)[idx];
}

void
MpdQueue::Move(unsigned dest, unsigned src)
{
	assert(src < size());
	assert(dest < size());
	assert(src != dest);

	auto &song = (*this)[src];

	if (src < dest) {
		memmove(&list->pdata[src],
			&list->pdata[src + 1],
			sizeof(list->pdata[0]) * (dest - src));
		list->pdata[dest] = &song;
	} else {
		memmove(&list->pdata[dest + 1],
			&list->pdata[dest],
			sizeof(list->pdata[0]) * (src - dest));
		list->pdata[dest] = &song;
	}
}

int
MpdQueue::Find(const struct mpd_song &song) const
{
	for (guint i = 0; i < size(); ++i) {
		if (&(*this)[i] == &song)
			return (gint)i;
	}

	return -1;
}

int
MpdQueue::FindId(unsigned id) const
{
	for (guint i = 0; i < size(); ++i) {
		const auto &song = (*this)[i];
		if (mpd_song_get_id(&song) == id)
			return (gint)i;
	}

	return -1;
}

int
MpdQueue::FindUri(const char *filename) const
{
	for (guint i = 0; i < size(); ++i) {
		const auto &song = (*this)[i];
		if (strcmp(mpd_song_get_uri(&song), filename) == 0)
			return (gint)i;
	}

	return -1;
}
