#ifndef SCREEN_H
#define SCREEN_H

#include "list_window.h"
#include "mpdclient.h"

#include <ncurses.h>
#include <glib.h>

#define IS_PLAYING(s) (s==MPD_STATUS_STATE_PLAY)
#define IS_PAUSED(s) (s==MPD_STATUS_STATE_PAUSE)
#define IS_STOPPED(s) (!(IS_PLAYING(s) | IS_PAUSED(s)))

#define MAX_SONGNAME_LENGTH   512

typedef struct {
	WINDOW *w;
	int rows, cols;
	int cur_action_id;
} window_t;

typedef struct {
	window_t top_window;
	window_t main_window;
	window_t progress_window;
	window_t status_window;

	/* GTime is equivalent to time_t */
	GTime start_timestamp;
	GTime status_timestamp;
	GTime input_timestamp;
	GTime last_cmd;

	int cols, rows;

	int mode;

	char *buf;
	size_t buf_size;

	char *findbuf;
	GList *find_history;

	int painted;
} screen_t;


typedef void (*screen_init_fn_t)(WINDOW *w, int cols, int rows);
typedef void (*screen_exit_fn_t)(void);
typedef void (*screen_open_fn_t)(screen_t *screen, mpdclient_t *c);
typedef void (*screen_close_fn_t)(void);
typedef void (*screen_resize_fn_t)(int cols, int rows);
typedef void (*screen_paint_fn_t)(screen_t *screen, mpdclient_t *c);
typedef void (*screen_update_fn_t)(screen_t *screen, mpdclient_t *c);
typedef int (*screen_cmd_fn_t)(screen_t *scr, mpdclient_t *c, command_t cmd);
typedef const char *(*screen_title_fn_t)(char *s, size_t size);
typedef list_window_t *(*screen_get_lw_fn_t) (void);

typedef struct {
	screen_init_fn_t   init;
	screen_exit_fn_t   exit;
	screen_open_fn_t   open;
	screen_close_fn_t  close;
	screen_resize_fn_t resize;
	screen_paint_fn_t  paint;
	screen_update_fn_t update;
	screen_cmd_fn_t    cmd;
	screen_title_fn_t  get_title;
	screen_get_lw_fn_t get_lw;
} screen_functions_t;

void
ncurses_init(void);

int screen_init(mpdclient_t *c);
int screen_exit(void);
void screen_resize(void);
void screen_status_message(const char *msg);
void screen_status_printf(const char *format, ...);
char *screen_error(void);
void screen_paint(mpdclient_t *c);
void screen_update(mpdclient_t *c);
void screen_idle(mpdclient_t *c);
void screen_cmd(mpdclient_t *c, command_t cmd);
gint screen_get_id(const char *name);


gint get_cur_mode_id(void);
int screen_get_mouse_event(mpdclient_t *c,
			   list_window_t *lw, int lw_length, 
			   unsigned long *bstate, int *row);

screen_functions_t *
get_screen_search(void);

screen_functions_t *
get_screen_browse(void);

screen_functions_t *
get_screen_playlist(void);

screen_functions_t *
get_screen_help(void);

screen_functions_t *
get_screen_lyrics(void);

screen_functions_t *
get_screen_keydef(void);

screen_functions_t *
get_screen_clock(void);

#endif
