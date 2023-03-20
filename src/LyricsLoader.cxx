// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "LyricsLoader.hxx"
#include "config.h"

#include <assert.h>

LyricsLoader::LyricsLoader() noexcept
	:plugins(plugin_list_load_directory(LYRICS_PLUGIN_DIR))
{
}

PluginCycle *
LyricsLoader::Load(EventLoop &event_loop,
		   const char *artist, const char *title,
		   PluginResponseHandler &handler) noexcept
{
	assert(artist != nullptr);
	assert(title != nullptr);

	const char *args[3] = { artist, title, nullptr };

	return plugin_run(event_loop, plugins, args, handler);
}
