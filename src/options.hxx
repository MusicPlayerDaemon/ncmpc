/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "config.h"
#include "defaults.hxx"

#include <glib.h>

typedef struct {
	char *host;
	char *username;
	char *password;
	char *config_file;
	char *key_file;
	char *list_format;
	char *search_format;
	char *status_format;
#ifndef NCMPC_MINI
	char *xterm_title_format;
	char *scroll_sep;
#endif
	char **screen_list;
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
	char *text_editor;
	bool text_editor_ask = false;
#endif
#ifdef ENABLE_CHAT_SCREEN
	char *chat_prefix;
#endif
	bool find_wrap = true;
	bool find_show_last_pattern;
	bool list_wrap;
	int scroll_offset = 0;
	bool auto_center;
	bool wide_cursor = true;
	bool hardware_cursor;

#ifdef ENABLE_COLORS
	bool enable_colors;
#endif
	bool audible_bell = true;
	bool visible_bell;
	bool bell_on_wrap = true;
	GTime status_message_time = 3;
#ifndef NCMPC_MINI
	bool enable_xterm_title;
#endif
#ifdef HAVE_GETMOUSE
	bool enable_mouse;
#endif
#ifndef NCMPC_MINI
	bool scroll = DEFAULT_SCROLL;
	bool visible_bitrate;
	bool welcome_screen_list = true;
	bool jump_prefix_only = true;
	bool second_column = true;
#endif
} options_t;

extern options_t options;

void options_init();
void options_deinit();

void options_parse(int argc, const char **argv);

#endif
