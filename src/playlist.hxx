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

#ifndef MPDCLIENT_PLAYLIST_H
#define MPDCLIENT_PLAYLIST_H

#include "Compiler.h"

#include <mpd/client.h>

#include <assert.h>
#include <glib.h>

struct MpdQueue {
	/* queue version number (obtained from mpd_status) */
	unsigned version = 0;

	/* the list */
	GPtrArray *list = g_ptr_array_sized_new(1024);;

	~MpdQueue();

	guint size() const {
		return list->len;
	}

	bool empty() const {
		return size() == 0;
	}

	/** remove and free all songs in the playlist */
	void clear();

	const struct mpd_song &operator[](guint i) const {
		assert(i < size());

		return *(const struct mpd_song *)g_ptr_array_index(list, i);
	}

	struct mpd_song &operator[](guint i) {
		assert(i < size());

		return *(struct mpd_song *)g_ptr_array_index(list, i);
	}

	gcc_pure
	const struct mpd_song *GetChecked(int i) const;

	void push_back(const struct mpd_song &song) {
		g_ptr_array_add(list, mpd_song_dup(&song));
	}

	void Set(guint i, const struct mpd_song &song) {
		assert(i < size());

		g_ptr_array_index(list, i) = mpd_song_dup(&song);
	}

	void Replace(guint i, const struct mpd_song &song) {
		mpd_song_free(&(*this)[i]);
		Set(i, song);
	}

	void RemoveIndex(guint i) {
		mpd_song_free((struct mpd_song *)g_ptr_array_remove_index(list, i));
	}

	void Move(unsigned dest, unsigned src);

	gcc_pure
	int Find(const struct mpd_song &song) const;

	gcc_pure
	int FindId(unsigned id) const;

	gcc_pure
	int FindUri(const char *uri) const;

	gcc_pure
	int FindUri(const struct mpd_song &song) const {
		return FindUri(mpd_song_get_uri(&song));
	}
};

#endif
