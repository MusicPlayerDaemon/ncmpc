/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#include "config.h"

#include <stdbool.h>
#include <glib.h>

#define MPD_HOST_ENV "MPD_HOST"
#define MPD_PORT_ENV "MPD_PORT"

typedef struct {
	char *host;
	char *username;
	char *password;
	char *config_file;
	char *key_file;
	char *list_format;
	char *status_format;
#ifndef NCMPC_MINI
	char *xterm_title_format;
	char *scroll_sep;
#endif
	char **screen_list;
	char *timedisplay_type;
	int port;
	int crossfade_time;
	int search_mode;
	int hide_cursor;
	int seek_time;
#ifdef ENABLE_LYRICS_SCREEN
	int lyrics_timeout;
	bool lyrics_autosave;
#endif
	bool find_wrap;
	bool find_show_last_pattern;
	bool list_wrap;
	int scroll_offset;
	bool auto_center;
	bool wide_cursor;
	bool hardware_cursor;

#ifdef ENABLE_COLORS
	bool enable_colors;
#endif
	bool audible_bell;
	bool visible_bell;
	bool bell_on_wrap;
	GTime status_message_time;
#ifndef NCMPC_MINI
	bool enable_xterm_title;
#endif
#ifdef HAVE_GETMOUSE
	bool enable_mouse;
#endif
#ifndef NCMPC_MINI
	bool scroll;
	bool visible_bitrate;
	bool welcome_screen_list;
	bool display_time;
	bool jump_prefix_only;
#endif
} options_t;

extern options_t options;

void options_init(void);
void options_deinit(void);

void options_parse(int argc, const char **argv);

#endif
