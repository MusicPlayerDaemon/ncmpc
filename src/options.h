#ifndef OPTIONS_H
#define OPTIONS_H

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
	int lyrics_timeout;
	bool reconnect;
	bool debug;
	bool find_wrap;
	bool find_show_last_pattern;
	bool list_wrap;
	bool auto_center;
	bool wide_cursor;
	bool enable_colors;
	bool audible_bell;
	bool visible_bell;
	bool enable_xterm_title;
	bool enable_mouse;
	bool scroll;
	bool visible_bitrate;
	bool welcome_screen_list;
} options_t;

#ifndef NO_GLOBAL_OPTIONS
extern options_t options;
#endif

options_t *options_init(void);
options_t *options_parse(int argc, const char **argv);

#endif
