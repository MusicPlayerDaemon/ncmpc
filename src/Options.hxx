/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#ifndef OPTIONS_HXX
#define OPTIONS_HXX

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

	std::chrono::steady_clock::duration hide_cursor;
	std::chrono::steady_clock::duration status_message_time = std::chrono::seconds(3);

	int port;
	int timeout_ms = 0;
	int crossfade_time = DEFAULT_CROSSFADE_TIME;
	int search_mode;
	int seek_time = 1;

	int scroll_offset = 0;

#ifdef ENABLE_CHAT_SCREEN
	std::string chat_prefix;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	std::string text_editor;
	std::chrono::steady_clock::duration lyrics_timeout = std::chrono::minutes(1);
	bool lyrics_autosave = false;
	bool lyrics_show_plugin = false;
	bool text_editor_ask = false;
#endif

	bool find_wrap = true;
	bool find_show_last_pattern;
	bool list_wrap;
	bool auto_center;
	bool wide_cursor = true;
	bool hardware_cursor;

#ifdef ENABLE_COLORS
	bool enable_colors = true;
#endif
	bool audible_bell = true;
	bool visible_bell;
	bool bell_on_wrap = true;
#ifndef NCMPC_MINI
	bool enable_xterm_title;
#endif
#ifdef HAVE_GETMOUSE
	bool enable_mouse;
#endif
#ifdef NCMPC_MINI
	static constexpr bool jump_prefix_only = true;
#else
	bool scroll = DEFAULT_SCROLL;
	bool visible_bitrate;
	bool welcome_screen_list = true;
	bool jump_prefix_only = true;
	bool second_column = true;
#endif

	CurrentTimeDisplay current_time_display = CurrentTimeDisplay::ELAPSED;
};

extern Options options;

void options_parse(int argc, const char **argv);

#endif
