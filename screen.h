#ifndef SCREEN_H
#define SCREEN_H
#include <ncurses.h>
#include "screen_utils.h"

#define TOP_HEADER_PREFIX "Music Player Client - "
#define TOP_HEADER_PLAY   TOP_HEADER_PREFIX "Playlist"
#define TOP_HEADER_FILE   TOP_HEADER_PREFIX "Browse"
#define TOP_HEADER_HELP   TOP_HEADER_PREFIX "Help    "
#define TOP_HEADER_SEARCH TOP_HEADER_PREFIX "Search  "

#define SCREEN_MIN_COLS 14
#define SCREEN_MIN_ROWS  5

#define IS_PLAYING(s) (s==MPD_STATUS_STATE_PLAY)

typedef enum
{
  SCREEN_PLAY_WINDOW = 0,
  SCREEN_FILE_WINDOW,
  SCREEN_HELP_WINDOW,
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
  time_t status_timestamp;

  list_window_t *playlist;
  list_window_t *filelist;
  list_window_t *helplist;

  int cols, rows;

  screen_mode_t mode;

  char *buf;
  size_t buf_size;

  char *findbuf;

  int painted;

} screen_t;



int screen_init(void);
int screen_exit(void);
void screen_resized(int sig);
void screen_status_message(char *msg);
void screen_status_printf(char *format, ...);
char *screen_error(void);
void screen_paint(mpd_client_t *c);
void screen_update(mpd_client_t *c);
void screen_cmd(mpd_client_t *c, command_t cmd);

#endif
