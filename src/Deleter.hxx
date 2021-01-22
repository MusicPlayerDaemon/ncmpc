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

#ifndef DELETER_HXX
#define DELETER_HXX

#include <mpd/client.h>

struct LibmpdclientDeleter {
	void operator()(struct mpd_song *song) const noexcept {
		mpd_song_free(song);
	}

	void operator()(struct mpd_output *o) const noexcept {
		mpd_output_free(o);
	}

#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	void operator()(struct mpd_partition *o) const noexcept {
		mpd_partition_free(o);
	}
#endif
};

#endif
