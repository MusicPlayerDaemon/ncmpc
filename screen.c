/* 
 * (c) 2004 by Kalle Wallin (kaw@linux.se)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

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

#define ENABLE_MP_FLAGS_DISPLAY
#undef  ENABLE_STATUS_LINE_CLOCK

#define DEFAULT_CROSSFADE_TIME 10

#define STATUS_MESSAGE_TIMEOUT 3
#define STATUS_LINE_MAX_SIZE   512

#ifdef ENABLE_KEYDEF_SCREEN
extern screen_functions_t *get_screen_keydef(void);
#endif

static screen_t *screen = NULL;
static screen_functions_t *mode_fn = NULL;

static void
switch_screen_mode(screen_mode_t new_mode, mpd_client_t *c)
{
  if( new_mode == screen->mode )
    return;

  /* close the old mode */
  if( mode_fn && mode_fn->close )
    mode_fn->close();

  /* get functions for the new mode */
  switch(new_mode)
    {
    case SCREEN_PLAY_WINDOW:
      mode_fn = get_screen_playlist();
      break;
    case SCREEN_FILE_WINDOW:
      mode_fn = get_screen_file();
      break;
    case SCREEN_HELP_WINDOW:
      mode_fn = get_screen_help();
      break;
#ifdef ENABLE_KEYDEF_SCREEN
    case SCREEN_KEYDEF_WINDOW:
      mode_fn = get_screen_keydef();
      break;
#endif
    default:
      break;
    }

 screen->mode = new_mode;
 screen->painted = 0;

 /* open the new mode */
 if( mode_fn && mode_fn->open )
   mode_fn->open(screen, c);

}

static void
paint_top_window(char *header, mpd_client_t *c, int clear)
{
  static int prev_volume = -1;
  static int prev_header_len = -1;
  WINDOW *w = screen->top_window.w;

  if(prev_header_len!=strlen(header))
    {
      prev_header_len = strlen(header);
      clear = 1;
    }

  if(clear)
    {
      wmove(w, 0, 0);
      wclrtoeol(w);
    }

  if(prev_volume!=c->status->volume || clear)
    {
      char buf[12];

      if( header[0] )
	{
	  wattron(w, A_BOLD);
	  mvwaddstr(w, 0, 0, header);
	  wattroff(w, A_BOLD);
	}
      else
	{
	  wattron(w, A_BOLD);
	  waddstr(w, get_key_names(CMD_SCREEN_HELP, FALSE));
	  wattroff(w, A_BOLD);
	  waddstr(w, ":Help  ");
	  wattron(w, A_BOLD);
	  waddstr(w, get_key_names(CMD_SCREEN_PLAY, FALSE));
	  wattroff(w, A_BOLD);
	  waddstr(w, ":Playlist  ");
	  wattron(w, A_BOLD);
	  waddstr(w, get_key_names(CMD_SCREEN_FILE, FALSE));
	  wattroff(w, A_BOLD);
	  waddstr(w, ":Browse");
	}
      if( c->status->volume==MPD_STATUS_NO_VOLUME )
	{
	  snprintf(buf, 12, "Volume n/a ");
	}
      else
	{
	  snprintf(buf, 12, "Volume %3d%%", c->status->volume); 
	}
      mvwaddstr(w, 0, screen->top_window.cols-12, buf);

      if( options.enable_colors )
	wattron(w, LINE_COLORS);

#ifdef ENABLE_MP_FLAGS_DISPLAY
      {
	char buf[4];

	buf[0] = 0;
	if( c->status->repeat )
	  strcat(buf, "r");
	if( c->status->random )
	  strcat(buf, "z");
	if( c->status->crossfade )
	  strcat(buf, "x");

	mvwhline(w, 1, 0, ACS_HLINE, screen->top_window.cols);
	if( buf[0] )
	  {
	    wmove(w,1,screen->top_window.cols-strlen(buf)-3);
	    waddch(w, '[');
	    wattron(w, A_BOLD);
	    waddstr(w, buf);
	    wattroff(w, A_BOLD);
	    waddch(w, ']');
	  }
      }
#else
      mvwhline(w, 1, 0, ACS_HLINE, screen->top_window.cols);
#endif

      wnoutrefresh(w);
    }
}

static void
paint_progress_window(mpd_client_t *c)
{
  double p;
  int width;
  int elapsedTime = c->status->elapsedTime;

  if( c->status==NULL || IS_STOPPED(c->status->state) )
    {
      mvwhline(screen->progress_window.w, 0, 0, ACS_HLINE, 
	       screen->progress_window.cols);
      wnoutrefresh(screen->progress_window.w);
      return;
    }

  if( c->seek_song_id == c->song_id )
    elapsedTime = c->seek_target_time;

  p = ((double) elapsedTime) / ((double) c->status->totalTime);
  
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
#ifdef ENABLE_STATUS_LINE_CLOCK
  else if( c->status->state == MPD_STATUS_STATE_STOP )
    {
      time_t timep;

      /* Note: setlocale(LC_TIME,"") should be used first */
      time(&timep);
      strftime(screen->buf, screen->buf_size, "%X ",  localtime(&timep));
      x = screen->status_window.cols - strlen(screen->buf) ;
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
      GList *list = g_list_first(screen->screen_list);

      /* close and exit all screens (playlist,browse,help...) */
      while( list )
	{
	  screen_functions_t *mode_fn = list->data;

	  if( mode_fn && mode_fn->close )
	    mode_fn->close();
	  if( mode_fn && mode_fn->exit )
	    mode_fn->exit();
	  list->data = NULL;
	  list=list->next;
	}
      g_list_free(screen->screen_list);

      g_free(screen->buf);
      g_free(screen->findbuf);
      g_free(screen);
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
  GList *list;

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

  screen = g_malloc(sizeof(screen_t));
  memset(screen, 0, sizeof(screen_t));
  screen->mode = SCREEN_PLAY_WINDOW;
  screen->cols = COLS;
  screen->rows = LINES;
  screen->buf  = g_malloc(screen->cols);
  screen->buf_size = screen->cols;
  screen->findbuf = NULL;
  screen->painted = 0;
  screen->start_timestamp = time(NULL);
  screen->input_timestamp = time(NULL);
  screen->last_cmd = CMD_NONE;

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

  //  leaveok(screen->main_window.w, TRUE); temporary disabled
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

  /* initialize screens */
  screen->screen_list = NULL;
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_playlist());
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_file());
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_help());
#ifdef ENABLE_KEYDEF_SCREEN
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_keydef());
#endif

  list = screen->screen_list;
  while( list )
    {
      screen_functions_t *fn = list->data;
      
      if( fn && fn->init )
	fn->init(screen->main_window.w, 
		 screen->main_window.cols,
		 screen->main_window.rows);
      
      list = list->next;
    }

  mode_fn = get_screen_playlist();

  return 0;
}

void 
screen_paint(mpd_client_t *c)
{
  /* paint the title/header window */
  if( mode_fn && mode_fn->get_title )
    paint_top_window(mode_fn->get_title(), c, 1);
  else
    paint_top_window("", c, 1);

  /* paint the main window */
  if( mode_fn && mode_fn->paint )
    mode_fn->paint(screen, c);
  
  paint_progress_window(c);
  paint_status_window(c);
  screen->painted = 1;
  wmove(screen->main_window.w, 0, 0);  
  wnoutrefresh(screen->main_window.w);

  /* tell curses to update */
  doupdate();
}

void 
screen_update(mpd_client_t *c)
{
  static int repeat = -1;
  static int random = -1;
  static int crossfade = -1;
  static int welcome = 1;
  list_window_t *lw = NULL;

  if( !screen->painted )
    return screen_paint(c);

  /* print a message if mpd status has changed */
  if( repeat<0 )
    {
      repeat = c->status->repeat;
      random = c->status->random;
      crossfade = c->status->crossfade;
    }
  if( repeat != c->status->repeat )
    screen_status_printf("Repeat is %s", 
			 c->status->repeat  ? "On" : "Off");
  if( random != c->status->random )
    screen_status_printf("Random is %s", 
			 c->status->random ? "On" : "Off");
  if( crossfade != c->status->crossfade )
    screen_status_printf("Crossfade %d seconds", c->status->crossfade);

  repeat = c->status->repeat;
  random = c->status->random;
  crossfade = c->status->crossfade;

  /* update title/header window */
  if( welcome && screen->last_cmd==CMD_NONE &&
      time(NULL)-screen->start_timestamp <= SCREEN_WELCOME_TIME)
    paint_top_window("", c, 0);
  else if( mode_fn && mode_fn->get_title )
    {
      paint_top_window(mode_fn->get_title(), c, 0);
      welcome = 0;
    }
  else
    paint_top_window("", c, 0);

  /* update the main window */
  if( mode_fn && mode_fn->paint )
    mode_fn->update(screen, c);

  if( mode_fn && mode_fn->get_lw )
    lw = mode_fn->get_lw();

  /* update progress window */
  paint_progress_window(c);

  /* update status window */
  paint_status_window(c);

  /* move the cursor to the selected row in the main window */
  if( lw )
    wmove(screen->main_window.w, LW_ROW(lw), 0);   
  else
    wmove(screen->main_window.w, 0, 0);   
  wnoutrefresh(screen->main_window.w);

  /* tell curses to update */
  doupdate();
}

void
screen_idle(mpd_client_t *c)
{
  if( c->seek_song_id ==  c->song_id &&
      (screen->last_cmd == CMD_SEEK_FORWARD || 
       screen->last_cmd == CMD_SEEK_BACKWARD) )
    {
      mpd_sendSeekCommand(c->connection, 
			  c->seek_song_id, 
			  c->seek_target_time);
      mpd_finishCommand(c->connection);
    }

  screen->last_cmd = CMD_NONE;
  c->seek_song_id = -1;
}

void 
screen_cmd(mpd_client_t *c, command_t cmd)
{
  int n;
  screen_mode_t new_mode = screen->mode;

  screen->input_timestamp = time(NULL);
  screen->last_cmd = cmd;

  if( mode_fn && mode_fn->cmd && mode_fn->cmd(screen, c, cmd) )
    return;

  switch(cmd)
    {
    case CMD_PLAY:
      mpd_sendPlayCommand(c->connection, play_get_selected());
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
    case CMD_SEEK_FORWARD:
      if( !IS_STOPPED(c->status->state) )
	{
	  if( c->seek_song_id != c->song_id )
	    {
	      c->seek_song_id = c->song_id;
	      c->seek_target_time = c->status->elapsedTime;
	    }
	  c->seek_target_time++;
	  if( c->seek_target_time < c->status->totalTime )
	    break;
	  c->seek_target_time=0;
	}
      /* fall through... */
    case CMD_TRACK_NEXT:
      if( !IS_STOPPED(c->status->state) )
	{
	  mpd_sendNextCommand(c->connection);
	  mpd_finishCommand(c->connection);
	}
      break;
    case CMD_SEEK_BACKWARD:
      if( !IS_STOPPED(c->status->state) )
	{
	  if( c->seek_song_id != c->song_id )
	    {
	      c->seek_song_id = c->song_id;
	      c->seek_target_time = c->status->elapsedTime;
	    }
	  c->seek_target_time--;
	  if( c->seek_target_time < 0 )
	    c->seek_target_time=0;
	}
      break;
    case CMD_TRACK_PREVIOUS:
      if( !IS_STOPPED(c->status->state) )
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
    case CMD_CROSSFADE:
      if( c->status->crossfade )
	n = 0;
      else
	n = DEFAULT_CROSSFADE_TIME;
      mpd_sendCrossfadeCommand(c->connection, n);
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
    case CMD_TOGGLE_AUTOCENTER:
      options.auto_center = !options.auto_center;
      screen_status_printf("Auto center mode: %s", 
			   options.auto_center ? "On" : "Off");
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
#ifdef ENABLE_KEYDEF_SCREEN 
    case CMD_SCREEN_KEYDEF:
      switch_screen_mode(SCREEN_KEYDEF_WINDOW, c);
      break;
#endif
    case CMD_QUIT:
      exit(EXIT_SUCCESS);
    default:
      break;
    }

}



