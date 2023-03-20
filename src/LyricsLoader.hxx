// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef LYRICS_LOADER_HXX
#define LYRICS_LOADER_HXX

#include "plugin.hxx"

class LyricsLoader {
	const PluginList plugins;

public:
	LyricsLoader() noexcept;

	PluginCycle *Load(EventLoop &event_loop,
			  const char *artist, const char *title,
			  PluginResponseHandler &handler) noexcept;
};

#endif
