#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <ncurses.h>

#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "options.h"
#include "screen.h"
#include "screen_play.h"
#include "screen_file.h"
#include "screen_help.h"
#include "screen_search.h"
#include "screen_utils.h"

#define STATUS_MESSAGE_TIMEOUT 3
#define STATUS_LINE_MAX_SIZE   512

static screen_t *screen = NULL;

static void
switch_screen_mode(screen_mode_t new_mode, mpd_client_t *c)
{
  if( new_mode == screen->mode )
    return;

 switch(screen->mode)
    {
    case SCREEN_PLAY_WINDOW:
      play_close(screen, c);
      break;
    case SCREEN_FILE_WINDOW:
      file_close(screen, c);
      break;
    case SCREEN_SEARCH_WINDOW:
      search_close(screen, c);
      break;
    case SCREEN_HELP_WINDOW:
      help_close(screen, c);
      break;
    }

 screen->mode = new_mode;
 screen->painted = 0;

 switch(screen->mode)
    {
    case SCREEN_PLAY_WINDOW:
      play_open(screen, c);
      break;
    case SCREEN_FILE_WINDOW:
      file_open(screen, c);
      break;
    case SCREEN_SEARCH_WINDOW:
      search_open(screen, c);
      break;
    case SCREEN_HELP_WINDOW:
      help_open(screen, c);
      break;
    }
}

static void
paint_top_window(char *header, int volume, int clear)
{
  static int prev_volume = -1;
  WINDOW *w = screen->top_window.w;

  if(clear)
    {
      wclear(w);
    }

  if(prev_volume!=volume || clear)
    {
      char buf[12];

      wattron(w, A_BOLD);
      mvwaddstr(w, 0, 0, header);
      wattroff(w, A_BOLD);
      if( volume==MPD_STATUS_NO_VOLUME )
	{
	  snprintf(buf, 12, "Volume n/a ");
	}
      else
	{
	  snprintf(buf, 12, "Volume %3d%%", volume); 
	}
      mvwaddstr(w, 0, screen->top_window.cols-12, buf);

      if( options.enable_colors )
	wattron(w, LINE_COLORS);
      mvwhline(w, 1, 0, ACS_HLINE, screen->top_window.cols);
      if( options.enable_colors )
	wattroff(w, LINE_COLORS);

      wnoutrefresh(w);
    }
}

static void
paint_progress_window(mpd_client_t *c)
{
  double p;
  int width;

  if( c->status==NULL || !IS_PLAYING(c->status->state) )
    {
      mvwhline(screen->progress_window.w, 0, 0, ACS_HLINE, 
	       screen->progress_window.cols);
      wnoutrefresh(screen->progress_window.w);
      return;
    }

  p = ((double) c->status->elapsedTime) / ((double) c->status->totalTime);
  
  width = (int) (p * (double) screen->progress_window.cols);
  mvwhline(screen->progress_window.w, 
	   0, 0,
	   ACS_HLINE, 
	   screen->progress_window.cols);
  whline(screen->progress_window.w, '=', width-1);
  mvwaddch(screen->progress_window.w, 0, width-1, 'O');
  wnoutrefresh(screen->progress_window.w);
}

static void 
paint_status_window(mpd_client_t *c)
{
  WINDOW *w = screen->status_window.w;
  mpd_Status *status = c->status;
  mpd_Song *song   = c->song;
  int x = 0;

  if( time(NULL) - screen->status_timestamp <= STATUS_MESSAGE_TIMEOUT )
    return;
  
  wmove(w, 0, 0);
  wclrtoeol(w);
  
  switch(status->state)
    {
    case MPD_STATUS_STATE_STOP:
      wattron(w, A_BOLD);
      waddstr(w, "Stopped! ");
      wattroff(w, A_BOLD);
      break;
    case MPD_STATUS_STATE_PLAY:
      wattron(w, A_BOLD);
      waddstr(w, "Playing:");
      wattroff(w, A_BOLD);
      break;
    case MPD_STATUS_STATE_PAUSE:
      wattron(w, A_BOLD);
      waddstr(w, "[Paused]");
      wattroff(w, A_BOLD);
      break;
    default:
      my_waddstr(w, 
		 "Warning: Music Player Daemon in unknown state!", 
		 ALERT_COLORS);
      break;
    }
  x += 10;

  if( (IS_PLAYING(status->state) || IS_PAUSED(status->state)) &&  song )
    {
      mvwaddstr(w, 0, x, mpc_get_song_name(song));
    }
  

  /* time */
  if( IS_PLAYING(status->state) || IS_PAUSED(status->state) )
    {
      x = screen->status_window.cols - strlen(screen->buf);

      snprintf(screen->buf, screen->buf_size, 
	       " [%i:%02i/%i:%02i] ",
	       status->elapsedTime/60, status->elapsedTime%60,
	       status->totalTime/60,   status->totalTime%60 );
      mvwaddstr(w, 0, x, screen->buf);
	
    }
#if 1
  else if( c->status->state == MPD_STATUS_STATE_STOP )
    {
      time_t timep;
      x = screen->status_window.cols - strlen(screen->buf);
      
      time(&timep);
      //strftime(screen->buf, screen->buf_size, "%X ",  localtime(&timep));
      strftime(screen->buf, screen->buf_size, "%R ",  localtime(&timep));
      mvwaddstr(w, 0, x, screen->buf);
    }
#endif
  

  wnoutrefresh(w);
}



int
screen_exit(void)
{
  endwin();
  if( screen )
    {
      screen->playlist = list_window_free(screen->playlist);
      screen->filelist = list_window_free(screen->filelist);
      screen->helplist = list_window_free(screen->helplist);
      free(screen->buf);
      free(screen->findbuf);
      free(screen);
      screen = NULL;
    }
  return 0;
}

void
screen_resized(int sig)
{
  screen_exit();
  if( COLS<SCREEN_MIN_COLS || LINES<SCREEN_MIN_ROWS )
    {
      fprintf(stderr, "Error: Screen to small!\n");
      exit(EXIT_FAILURE);
    }
  screen_init();
}

void 
screen_status_message(char *msg)
{
  WINDOW *w = screen->status_window.w;

  wmove(w, 0, 0);
  wclrtoeol(w);
  wattron(w, A_BOLD);
  my_waddstr(w, msg, ALERT_COLORS);
  wattroff(w, A_BOLD);
  wnoutrefresh(w);
  screen->status_timestamp = time(NULL);
}

void 
screen_status_printf(char *format, ...)
{
  char buffer[STATUS_LINE_MAX_SIZE];
  va_list ap;
  
  va_start(ap,format);
  vsnprintf(buffer,sizeof(buffer),format,ap);
  va_end(ap);
  screen_status_message(buffer);
}

int
screen_init(void)
{
  /* initialize the curses library */
  initscr();        
  if( has_colors() )
    {
      start_color();
      if( options.enable_colors )
	{
	  init_pair(1, options.title_color,    options.bg_color);
	  init_pair(2, options.line_color,     options.bg_color);
	  init_pair(3, options.list_color,     options.bg_color);
	  init_pair(4, options.progress_color, options.bg_color);
	  init_pair(5, options.status_color,   options.bg_color);
	  init_pair(6, options.alert_color,    options.bg_color);
	}
      else
	use_default_colors();
    }
  else if( options.enable_colors )
    {
      fprintf(stderr, "Terminal lacks color capabilities.\n");
      options.enable_colors = 0;
    }

  /* tell curses not to do NL->CR/NL on output */
  nonl();          
  /* take input chars one at a time, no wait for \n */  
  cbreak();       
  /* don't echo input */
  noecho();    
  /* set cursor invisible */     
  curs_set(0);     
  /* return from getch() without blocking */
  //  nodelay(stdscr, TRUE); 
  keypad(stdscr, TRUE);  
  timeout(SCREEN_TIMEOUT);

  if( COLS<SCREEN_MIN_COLS || LINES<SCREEN_MIN_ROWS )
    {
      fprintf(stderr, "Error: Screen to small!\n");
      exit(EXIT_FAILURE);
    }

  screen = malloc(sizeof(screen_t));
  memset(screen, 0, sizeof(screen_t));
  screen->mode = SCREEN_PLAY_WINDOW;
  screen->cols = COLS;
  screen->rows = LINES;
  screen->buf  = malloc(screen->cols);
  screen->buf_size = screen->cols;
  screen->findbuf = NULL;
  screen->painted = 0;

  /* create top window */
  screen->top_window.rows = 2;
  screen->top_window.cols = screen->cols;
  screen->top_window.w = newwin(screen->top_window.rows, 
				  screen->top_window.cols,
				  0, 0);
  leaveok(screen->top_window.w, TRUE);
  keypad(screen->top_window.w, TRUE);  

  /* create main window */
  screen->main_window.rows = screen->rows-4;
  screen->main_window.cols = screen->cols;
  screen->main_window.w = newwin(screen->main_window.rows, 
				 screen->main_window.cols,
				 2, 
				 0);
  screen->playlist = list_window_init( screen->main_window.w,
				       screen->main_window.cols,
				       screen->main_window.rows );
  screen->filelist = list_window_init( screen->main_window.w,
				       screen->main_window.cols,
				       screen->main_window.rows );
  screen->helplist = list_window_init( screen->main_window.w,
				       screen->main_window.cols,
				       screen->main_window.rows );

  leaveok(screen->main_window.w, TRUE);
  keypad(screen->main_window.w, TRUE);  

  /* create progress window */
  screen->progress_window.rows = 1;
  screen->progress_window.cols = screen->cols;
  screen->progress_window.w = newwin(screen->progress_window.rows, 
				     screen->progress_window.cols,
				     screen->rows-2, 
				     0);
  leaveok(screen->progress_window.w, TRUE);
  
  /* create status window */
  screen->status_window.rows = 1;
  screen->status_window.cols = screen->cols;
  screen->status_window.w = newwin(screen->status_window.rows, 
				   screen->status_window.cols,
				   screen->rows-1, 
				   0);

  leaveok(screen->status_window.w, FALSE);
  keypad(screen->status_window.w, TRUE);  

  if( options.enable_colors )
    {
      /* set background attributes */
      wbkgd(screen->main_window.w, LIST_COLORS);
      wbkgd(screen->top_window.w, TITLE_COLORS);
      wbkgd(screen->progress_window.w, PROGRESS_COLORS);
      wbkgd(screen->status_window.w, STATUS_COLORS);
    }

  return 0;
}

void 
screen_paint(mpd_client_t *c)
{
  switch(screen->mode)
    {
    case SCREEN_PLAY_WINDOW:
      paint_top_window(TOP_HEADER_PLAY, c->status->volume, 1);
      play_paint(screen, c);      
      break;
    case SCREEN_FILE_WINDOW:
      paint_top_window(file_get_header(c), c->status->volume, 1);
      file_paint(screen, c);      
      break;
    case SCREEN_SEARCH_WINDOW:
      paint_top_window(TOP_HEADER_SEARCH, c->status->volume, 1);
      search_paint(screen, c);      
      break;
    case SCREEN_HELP_WINDOW:
      paint_top_window(TOP_HEADER_PLAY, c->status->volume, 1);
      help_paint(screen, c);      
      break;
    }

  paint_progress_window(c);
  paint_status_window(c);
  screen->painted = 1;
  doupdate();
}

void 
screen_update(mpd_client_t *c)
{
  if( !screen->painted )
    return screen_paint(c);

  switch(screen->mode)
    {
    case SCREEN_PLAY_WINDOW:
      paint_top_window(TOP_HEADER_PLAY, c->status->volume, 0);
      play_update(screen, c);
      break;
    case SCREEN_FILE_WINDOW:
      paint_top_window(file_get_header(c), c->status->volume, 0);
      file_update(screen, c);
      break;
    case SCREEN_SEARCH_WINDOW:
      paint_top_window(TOP_HEADER_SEARCH, c->status->volume, 0);
      search_update(screen, c);
      break;
    case SCREEN_HELP_WINDOW:
      paint_top_window(TOP_HEADER_HELP, c->status->volume, 0);
      help_update(screen, c);
      break;
    }
  paint_progress_window(c);
  paint_status_window(c);
  doupdate();
}

void 
screen_cmd(mpd_client_t *c, command_t cmd)
{
  int n;
  screen_mode_t new_mode = screen->mode;

  switch(screen->mode)
    {
    case SCREEN_PLAY_WINDOW:
      if( play_cmd(screen, c, cmd) )
	return;
      break;
    case SCREEN_FILE_WINDOW:
      if( file_cmd(screen, c, cmd) )
	return;
      break;
    case SCREEN_SEARCH_WINDOW:
      if( search_cmd(screen, c, cmd) )
	return;
      break;
    case SCREEN_HELP_WINDOW:
      if( help_cmd(screen, c, cmd) )
	return;
      break;
    }

  switch(cmd)
    {
    case CMD_PLAY:
      mpd_sendPlayCommand(c->connection, screen->playlist->selected);
      mpd_finishCommand(c->connection);
      break;
    case CMD_PAUSE:
      mpd_sendPauseCommand(c->connection);
      mpd_finishCommand(c->connection);
      break;
    case CMD_STOP:
      mpd_sendStopCommand(c->connection);
      mpd_finishCommand(c->connection);
      break;
    case CMD_TRACK_NEXT:
      if( IS_PLAYING(c->status->state) )
	{
	  mpd_sendNextCommand(c->connection);
	  mpd_finishCommand(c->connection);
	}
      break;
    case CMD_TRACK_PREVIOUS:
      if( IS_PLAYING(c->status->state) )
	{
	  mpd_sendPrevCommand(c->connection);
	  mpd_finishCommand(c->connection);
	}
      break;
    case CMD_SHUFFLE:
      mpd_sendShuffleCommand(c->connection);
      mpd_finishCommand(c->connection);
      screen_status_message("Shuffled playlist!");
      break;
    case CMD_CLEAR:
      mpd_sendClearCommand(c->connection);
      mpd_finishCommand(c->connection);
      file_clear_highlights(c);
      screen_status_message("Cleared playlist!");
      break;
    case CMD_REPEAT:
      n = !c->status->repeat;
      mpd_sendRepeatCommand(c->connection, n);
      mpd_finishCommand(c->connection);
      break;
    case CMD_RANDOM:
      n = !c->status->random;
      mpd_sendRandomCommand(c->connection, n);
      mpd_finishCommand(c->connection);
      break;
    case CMD_VOLUME_UP:
      if( c->status->volume!=MPD_STATUS_NO_VOLUME && c->status->volume<100 )
	{
	  c->status->volume=c->status->volume+1;
	  mpd_sendSetvolCommand(c->connection, c->status->volume  );
	  mpd_finishCommand(c->connection);
	  screen_status_printf("Volume %d%%", c->status->volume);
	}
      break;
    case CMD_VOLUME_DOWN:
      if( c->status->volume!=MPD_STATUS_NO_VOLUME && c->status->volume>0 )
	{
	  c->status->volume=c->status->volume-1;
	  mpd_sendSetvolCommand(c->connection, c->status->volume  );
	  mpd_finishCommand(c->connection);
	  screen_status_printf("Volume %d%%", c->status->volume);
	}
      break;
    case CMD_TOGGLE_FIND_WRAP:
      options.find_wrap = !options.find_wrap;
      screen_status_printf("Find mode: %s", 
			   options.find_wrap ? "Wrapped" : "Normal");
      break;
    case CMD_SCREEN_PREVIOUS:
      if( screen->mode > SCREEN_PLAY_WINDOW )
	new_mode = screen->mode - 1;
      else
	new_mode = SCREEN_HELP_WINDOW-1;
      switch_screen_mode(new_mode, c);
      break;
    case CMD_SCREEN_NEXT:
      new_mode = screen->mode + 1;
      if( new_mode >= SCREEN_HELP_WINDOW )
	new_mode = SCREEN_PLAY_WINDOW;
      switch_screen_mode(new_mode, c);
      break;
    case CMD_SCREEN_PLAY:
      switch_screen_mode(SCREEN_PLAY_WINDOW, c);
      break;
    case CMD_SCREEN_FILE:
      switch_screen_mode(SCREEN_FILE_WINDOW, c);
      break;
    case CMD_SCREEN_SEARCH:
      switch_screen_mode(SCREEN_SEARCH_WINDOW, c);
      break;
    case CMD_SCREEN_HELP:
      switch_screen_mode(SCREEN_HELP_WINDOW, c);
      break;
    case CMD_QUIT:
      exit(EXIT_SUCCESS);
    default:
      break;
    }

}



