#ifndef OPTIONS_H
#define OPTIONS_H

#include "config.h"

#include <stdbool.h>

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
	char *xterm_title_format;
	char *scroll_sep;
	char **screen_list;
	char *timedisplay_type;
	int port;
	int crossfade_time;
	int search_mode;
	int hide_cursor;
	int seek_time;
#ifdef ENABLE_LYRICS_SCREEN
	int lyrics_timeout;
#endif
	bool find_wrap;
	bool find_show_last_pattern;
	bool list_wrap;
	bool auto_center;
	bool wide_cursor;
#ifdef ENABLE_COLORS
	bool enable_colors;
#endif
	bool audible_bell;
	bool visible_bell;
	bool enable_xterm_title;
#ifdef HAVE_GETMOUSE
	bool enable_mouse;
#endif
	bool scroll;
	bool visible_bitrate;
	bool welcome_screen_list;
} options_t;

extern options_t options;

void options_init(void);
void options_parse(int argc, const char **argv);

#endif
