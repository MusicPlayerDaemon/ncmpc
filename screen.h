#ifndef SCREEN_H
#define SCREEN_H
#include <ncurses.h>
#include "list_window.h"

/* top window headers */
#define TOP_HEADER_PREFIX "Music Player Client - "
#define TOP_HEADER_PLAY   TOP_HEADER_PREFIX "Playlist"
#define TOP_HEADER_FILE   TOP_HEADER_PREFIX "Browse"
#define TOP_HEADER_HELP   TOP_HEADER_PREFIX "Help    "
#define TOP_HEADER_SEARCH TOP_HEADER_PREFIX "Search  "

/* colors */
#define TITLE_COLORS     COLOR_PAIR(1)
#define LINE_COLORS      COLOR_PAIR(2)
#define LIST_COLORS      COLOR_PAIR(3)
#define PROGRESS_COLORS  COLOR_PAIR(4)
#define STATUS_COLORS    COLOR_PAIR(5)
#define ALERT_COLORS     COLOR_PAIR(6)

/* minumum window size */
#define SCREEN_MIN_COLS 14
#define SCREEN_MIN_ROWS  5

/* timeout for non blocking read [ms] */
#define SCREEN_TIMEOUT 500

/* welcome message time [s] */
#define SCREEN_WELCOME_TIME 10

#define IS_PLAYING(s) (s==MPD_STATUS_STATE_PLAY)
#define IS_PAUSED(s) (s==MPD_STATUS_STATE_PAUSE)
#define IS_STOPPED(s) (!(IS_PLAYING(s) | IS_PAUSED(s)))


typedef enum
{
  SCREEN_PLAY_WINDOW = 0,
  SCREEN_FILE_WINDOW,
  SCREEN_HELP_WINDOW,
  SCREEN_KEYDEF_WINDOW,
  SCREEN_SEARCH_WINDOW

} screen_mode_t;

typedef struct
{
  WINDOW *w;
  int rows, cols;

} window_t;



typedef struct
{
  window_t top_window;
  window_t main_window;
  window_t progress_window;
  window_t status_window;

  GList *screen_list;

  time_t start_timestamp;
  time_t status_timestamp;
  time_t input_timestamp;
  command_t last_cmd;

  int cols, rows;

  screen_mode_t mode;

  char *buf;
  size_t buf_size;

  char *findbuf;

  int painted;

} screen_t;


typedef void (*screen_init_fn_t)   (WINDOW *w, int cols, int rows);
typedef void (*screen_exit_fn_t)   (void);
typedef void (*screen_open_fn_t)   (screen_t *screen, mpd_client_t *c);
typedef void (*screen_close_fn_t)  (void);
typedef void (*screen_paint_fn_t)  (screen_t *screen, mpd_client_t *c);
typedef void (*screen_update_fn_t) (screen_t *screen, mpd_client_t *c);
typedef int (*screen_cmd_fn_t) (screen_t *scr, mpd_client_t *c, command_t cmd);
typedef char * (*screen_title_fn_t) (void);
typedef list_window_t * (*screen_get_lw_fn_t) (void);

typedef struct
{
  screen_init_fn_t   init;
  screen_exit_fn_t   exit;
  screen_open_fn_t   open;
  screen_close_fn_t  close;
  screen_paint_fn_t  paint;
  screen_update_fn_t update;
  screen_cmd_fn_t    cmd;
  screen_title_fn_t  get_title;
  screen_get_lw_fn_t get_lw;

} screen_functions_t;


int screen_init(void);
int screen_exit(void);
void screen_resized(int sig);
void screen_status_message(char *msg);
void screen_status_printf(char *format, ...);
char *screen_error(void);
void screen_paint(mpd_client_t *c);
void screen_update(mpd_client_t *c);
void screen_idle(mpd_client_t *c);
void screen_cmd(mpd_client_t *c, command_t cmd);

#endif
