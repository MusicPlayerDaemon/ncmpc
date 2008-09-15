#ifndef OPTIONS_H
#define OPTIONS_H

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
	gboolean reconnect;
	gboolean debug;
	gboolean find_wrap;
	gboolean find_show_last_pattern;
	gboolean list_wrap;
	gboolean auto_center;
	gboolean wide_cursor;
	gboolean enable_colors;
	gboolean audible_bell;
	gboolean visible_bell;
	gboolean enable_xterm_title;
	gboolean enable_mouse;
	gboolean scroll;
} options_t;

extern options_t options;

options_t *options_init(void);
options_t *options_parse(int argc, const char **argv);

#endif
