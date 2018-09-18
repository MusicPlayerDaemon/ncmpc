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

#ifndef OPTIONS_HXX
#define OPTIONS_HXX

#include "config.h"
#include "defaults.hxx"

#include <vector>
#include <string>
#include <chrono>

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
	bool display_remaining_time;
	int port;
	int timeout_ms = 0;
	int crossfade_time = DEFAULT_CROSSFADE_TIME;
	int search_mode;
	int hide_cursor;
	int seek_time = 1;
#ifdef ENABLE_LYRICS_SCREEN
	int lyrics_timeout = DEFAULT_LYRICS_TIMEOUT;
	bool lyrics_autosave = false;
	bool lyrics_show_plugin = false;
	std::string text_editor;
	bool text_editor_ask = false;
#endif
#ifdef ENABLE_CHAT_SCREEN
	std::string chat_prefix;
#endif
	bool find_wrap = true;
	bool find_show_last_pattern;
	bool list_wrap;
	int scroll_offset = 0;
	bool auto_center;
	bool wide_cursor = true;
	bool hardware_cursor;

#ifdef ENABLE_COLORS
	bool enable_colors = true;
#endif
	bool audible_bell = true;
	bool visible_bell;
	bool bell_on_wrap = true;
	std::chrono::seconds status_message_time = std::chrono::seconds(3);
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
};

extern Options options;

void options_parse(int argc, const char **argv);

#endif
