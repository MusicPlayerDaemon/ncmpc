// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "defaults.hxx"

#include <mpd/tag.h>

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>

enum class CurrentTimeDisplay : uint8_t {
	ELAPSED,
	REMAINING,
	NONE,
};

struct Options {
	std::string host;
	std::string password;
	std::string config_file;
	std::string key_file;
	std::string list_format = DEFAULT_LIST_FORMAT;
	std::string search_format;
	std::string status_format = DEFAULT_STATUS_FORMAT;
#ifndef NCMPC_MINI
	std::string xterm_title_format;
	std::string scroll_sep = DEFAULT_SCROLL_SEP;
#endif
	std::vector<std::string> screen_list = DEFAULT_SCREEN_LIST;

#ifdef ENABLE_LIBRARY_PAGE
	std::vector<enum mpd_tag_type> library_page_tags{MPD_TAG_ARTIST, MPD_TAG_ALBUM};
#endif

	std::chrono::steady_clock::duration hide_cursor{0};
	std::chrono::steady_clock::duration status_message_time = std::chrono::seconds(3);

	int port = 0;
	int timeout_ms = 0;
	int crossfade_time = DEFAULT_CROSSFADE_TIME;
	int search_mode = 0;
	int seek_time = 1;

#ifdef ENABLE_CHAT_SCREEN
	std::string chat_prefix;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	std::string text_editor;
	std::chrono::steady_clock::duration lyrics_timeout = std::chrono::minutes(1);
	bool lyrics_autosave = false;
	bool lyrics_show_plugin = false;
#endif

	bool auto_center = false;

#ifndef NCMPC_MINI
	bool enable_xterm_title = false;
#endif
#ifdef HAVE_GETMOUSE
	bool enable_mouse = false;
#endif
#ifndef NCMPC_MINI
	bool scroll = DEFAULT_SCROLL;
	bool visible_bitrate = false;
	bool welcome_screen_list = true;
	bool second_column = true;
#endif

	CurrentTimeDisplay current_time_display = CurrentTimeDisplay::ELAPSED;
};

extern Options options;

void options_parse(int argc, const char **argv);

void options_env();
