// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
