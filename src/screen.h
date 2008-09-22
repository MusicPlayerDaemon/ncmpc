#ifndef SCREEN_H
#define SCREEN_H

#include "mpdclient.h"
#include "command.h"

#include <ncurses.h>
#include <glib.h>

#define IS_PLAYING(s) (s==MPD_STATUS_STATE_PLAY)
#define IS_PAUSED(s) (s==MPD_STATUS_STATE_PAUSE)
#define IS_STOPPED(s) (!(IS_PLAYING(s) | IS_PAUSED(s)))

#define MAX_SONGNAME_LENGTH   512

struct window {
	WINDOW *w;
	unsigned rows, cols;
};

typedef struct screen {
	struct window top_window;
	struct window main_window;
	struct window progress_window;
	struct window status_window;

	/* GTime is equivalent to time_t */
	GTime start_timestamp;
	GTime status_timestamp;

	command_t last_cmd;

	unsigned cols, rows;

	int mode;

	char *buf;
	size_t buf_size;

	char *findbuf;
	GList *find_history;

	int painted;
} screen_t;


typedef void (*screen_init_fn_t)(WINDOW *w, int cols, int rows);
typedef void (*screen_exit_fn_t)(void);
typedef void (*screen_open_fn_t)(struct screen *screen, mpdclient_t *c);
typedef void (*screen_close_fn_t)(void);
typedef void (*screen_resize_fn_t)(int cols, int rows);
typedef void (*screen_paint_fn_t)(struct screen *screen, mpdclient_t *c);
typedef void (*screen_update_fn_t)(struct screen *screen, mpdclient_t *c);
typedef int (*screen_cmd_fn_t)(struct screen *scr, mpdclient_t *c, command_t cmd);
typedef const char *(*screen_title_fn_t)(char *s, size_t size);

typedef struct screen_functions {
	screen_init_fn_t   init;
	screen_exit_fn_t   exit;
	screen_open_fn_t   open;
	screen_close_fn_t  close;
	screen_resize_fn_t resize;
	screen_paint_fn_t  paint;
	screen_update_fn_t update;
	screen_cmd_fn_t    cmd;
	screen_title_fn_t  get_title;
} screen_functions_t;

void screen_init(mpdclient_t *c);
void screen_exit(void);
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
int screen_get_mouse_event(mpdclient_t *c, unsigned long *bstate, int *row);

#endif
