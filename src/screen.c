/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
#include <locale.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "mpdclient.h"
#include "command.h"
#include "options.h"
#include "colors.h"
#include "strfsong.h"
#include "wreadln.h"
#include "screen.h"
#include "screen_utils.h"

#define ENABLE_STATUS_LINE_CLOCK
#define ENABLE_SCROLLING

#define CROSSFADE_TIME 10

#define STATUS_MESSAGE_TIMEOUT 3
#define STATUS_LINE_MAX_SIZE   512

/* screens */
extern screen_functions_t *get_screen_playlist(void);
extern screen_functions_t *get_screen_browse(void);
extern screen_functions_t *get_screen_help(void);

#ifdef ENABLE_KEYDEF_SCREEN
extern screen_functions_t *get_screen_keydef(void);
#endif
#ifdef ENABLE_CLOCK_SCREEN
extern screen_functions_t *get_screen_clock(void);
#endif

static gboolean welcome = TRUE;
static screen_t *screen = NULL;
static screen_functions_t *mode_fn = NULL;
static int seek_id = -1;
static int seek_target_time = 0;


static void
switch_screen_mode(screen_mode_t new_mode, mpdclient_t *c)
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
      mode_fn = get_screen_browse();
      break;
    case SCREEN_HELP_WINDOW:
      mode_fn = get_screen_help();
      break;
#ifdef ENABLE_KEYDEF_SCREEN
    case SCREEN_KEYDEF_WINDOW:
      mode_fn = get_screen_keydef();
      break;
#endif
#ifdef ENABLE_CLOCK_SCREEN
    case SCREEN_CLOCK_WINDOW:
      mode_fn = get_screen_clock();
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
paint_top_window(char *header, mpdclient_t *c, int clear)
{
  char flags[4];
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
      char buf[32];

      if( header[0] )
	{
	  colors_use(w, COLOR_TITLE_BOLD);
	  mvwaddstr(w, 0, 0, header);
	}
      else
	{
	  colors_use(w, COLOR_TITLE_BOLD);
	  waddstr(w, get_key_names(CMD_SCREEN_HELP, FALSE));
	  colors_use(w, COLOR_TITLE);
	  waddstr(w, _(":Help  "));
	  colors_use(w, COLOR_TITLE_BOLD);
	  waddstr(w, get_key_names(CMD_SCREEN_PLAY, FALSE));
	  colors_use(w, COLOR_TITLE);
	  waddstr(w, _(":Playlist  "));
	  colors_use(w, COLOR_TITLE_BOLD);
	  waddstr(w, get_key_names(CMD_SCREEN_FILE, FALSE));
	  colors_use(w, COLOR_TITLE);
	  waddstr(w, _(":Browse"));
	}
      if( c->status->volume==MPD_STATUS_NO_VOLUME )
	{
	  snprintf(buf, 32, _("Volume n/a "));
	}
      else
	{
	  snprintf(buf, 32, _(" Volume %d%%"), c->status->volume); 
	}
      colors_use(w, COLOR_TITLE);
      mvwaddstr(w, 0, screen->top_window.cols-strlen(buf), buf);

      flags[0] = 0;
      if( c->status->repeat )
	strcat(flags, "r");
      if( c->status->random )
	strcat(flags, "z");
      if( c->status->crossfade )
	strcat(flags, "x");
      if( c->status->updatingDb )
	strcat(flags, "U");
      colors_use(w, COLOR_LINE);
      mvwhline(w, 1, 0, ACS_HLINE, screen->top_window.cols);
      if( flags[0] )
	{
	  wmove(w,1,screen->top_window.cols-strlen(flags)-3);
	  waddch(w, '[');
	  colors_use(w, COLOR_LINE_BOLD);
	  waddstr(w, flags);
	  colors_use(w, COLOR_LINE);
	  waddch(w, ']');
	}
      wnoutrefresh(w);
    }
}

static void
paint_progress_window(mpdclient_t *c)
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

  if( c->song && seek_id == c->song->id )
    elapsedTime = seek_target_time;

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
paint_status_window(mpdclient_t *c)
{
  WINDOW *w = screen->status_window.w;
  mpd_Status *status = c->status;
  mpd_Song *song   = c->song;
  int elapsedTime = c->status->elapsedTime;
  char *str = NULL;
  int x = 0;

  if( time(NULL) - screen->status_timestamp <= STATUS_MESSAGE_TIMEOUT )
    return;
  
  
  wmove(w, 0, 0);
  wclrtoeol(w);
  colors_use(w, COLOR_STATUS_BOLD);
  
  switch(status->state)
    {
    case MPD_STATUS_STATE_PLAY:
      str = _("Playing:");
      break;
    case MPD_STATUS_STATE_PAUSE:
      str = _("[Paused]");
      break;
    case MPD_STATUS_STATE_STOP:
    default:
      break;
    }

  if( str )
    {
      waddstr(w, str);
      x += strlen(str)+1;
    }

  /* create time string */
  memset(screen->buf, 0, screen->buf_size);
  if( IS_PLAYING(status->state) || IS_PAUSED(status->state) )
    {
      if( status->totalTime > 0 )
	{
	  if( c->song && seek_id == c->song->id )
	    elapsedTime = seek_target_time;
	  snprintf(screen->buf, screen->buf_size, 
		   " [%i:%02i/%i:%02i]",
		   elapsedTime/60, elapsedTime%60,
		   status->totalTime/60,   status->totalTime%60 );
	}
      else
	{
	  snprintf(screen->buf, screen->buf_size,  " [%d kbps]", status->bitRate );
	}
    }
#ifdef ENABLE_STATUS_LINE_CLOCK
  else
    {
      time_t timep;

      time(&timep);
      strftime(screen->buf, screen->buf_size, "%X ",localtime(&timep));
    }
#endif

  /* display song */
  if( (IS_PLAYING(status->state) || IS_PAUSED(status->state)) )
    {
      char songname[STATUS_LINE_MAX_SIZE];
      int width = COLS-x-strlen(screen->buf);

      if( song )
	strfsong(songname, STATUS_LINE_MAX_SIZE, STATUS_FORMAT, song);
      else
	songname[0] = '\0';

      colors_use(w, COLOR_STATUS);
#ifdef ENABLE_SCROLLING
      if( strlen(songname) > width )
	{
	  static  scroll_state_t st = { 0, 0 };
	  char *tmp = strscroll(songname, " *** ", width, &st);

	  strcpy(songname, tmp);
	  g_free(tmp);	  
	}
#endif
      mvwaddnstr(w, 0, x, songname, width);
    } 

  /* distplay time string */
  if( screen->buf[0] )
    {
      x = screen->status_window.cols - strlen(screen->buf);
      colors_use(w, COLOR_STATUS_TIME);
      mvwaddstr(w, 0, x, screen->buf);
    }

  wnoutrefresh(w);
}

GList *
screen_free_string_list(GList *list)
{
  GList *l = g_list_first(list);
  
  while(l)
    {
      g_free(l->data);
      l->data = NULL;
      l=l->next;
    }
  g_list_free(list);
  return NULL;
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
      screen_free_string_list(screen->find_history);
      g_free(screen->buf);
      g_free(screen->findbuf);
      
      g_free(screen);
      screen = NULL;
    }
  return 0;
}

void
screen_resize(void)
{
  GList *list;

  D("Resize rows %d->%d, cols %d->%d\n",screen->rows,LINES,screen->cols,COLS);
  if( COLS<SCREEN_MIN_COLS || LINES<SCREEN_MIN_ROWS )
    {
      screen_exit();
      fprintf(stderr, _("Error: Screen to small!\n"));
      exit(EXIT_FAILURE);
    }

  resizeterm(LINES, COLS);

  screen->cols = COLS;
  screen->rows = LINES;

  /* top window */
  screen->top_window.cols = screen->cols;
  wresize(screen->top_window.w, 2, screen->cols);

  /* main window */
  screen->main_window.cols = screen->cols;
  screen->main_window.rows = screen->rows-4;
  wresize(screen->main_window.w, screen->main_window.rows, screen->cols);
  wclear(screen->main_window.w);

  /* progress window */
  screen->progress_window.cols = screen->cols;
  wresize(screen->progress_window.w, 1, screen->cols);
  mvwin(screen->progress_window.w, screen->rows-2, 0);

  /* status window */
  screen->status_window.cols = screen->cols;
  wresize(screen->status_window.w, 1, screen->cols);
  mvwin(screen->status_window.w, screen->rows-1, 0);

  screen->buf_size = screen->cols;
  g_free(screen->buf);
  screen->buf = g_malloc(screen->cols);

  list = g_list_first(screen->screen_list);
  while( list )
    {
      screen_functions_t *mode_fn = list->data;

      if( mode_fn && mode_fn->resize )
	mode_fn->resize(screen->main_window.cols, screen->main_window.rows);

      list=list->next;
    }

  /* ? - without this the cursor becomes visible with aterm & Eterm */
  curs_set(1);
  curs_set(0);     

  screen->painted = 0;
}

void 
screen_status_message(char *msg)
{
  WINDOW *w = screen->status_window.w;

  wmove(w, 0, 0);
  wclrtoeol(w);
  colors_use(w, COLOR_STATUS_ALERT);
  waddstr(w, msg);
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
screen_init(mpdclient_t *c)
{
  GList *list;

  /* initialize the curses library */
  initscr();
  /* initialize color support */
  colors_start();
  /* tell curses not to do NL->CR/NL on output */
  nonl();          
  /*  use raw mode (ignore interrupt,quit,suspend, and flow control ) */
  raw();
  /* don't echo input */
  noecho();    
  /* set cursor invisible */     
  curs_set(0);     
  /* enable extra keys */
  keypad(stdscr, TRUE);  
  /* return from getch() without blocking */
  timeout(SCREEN_TIMEOUT);
  

  if( COLS<SCREEN_MIN_COLS || LINES<SCREEN_MIN_ROWS )
    {
      fprintf(stderr, _("Error: Screen to small!\n"));
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
      wbkgd(stdscr, COLOR_PAIR(COLOR_LIST)); 
      wbkgd(screen->main_window.w,     COLOR_PAIR(COLOR_LIST));
      wbkgd(screen->top_window.w,      COLOR_PAIR(COLOR_TITLE));
      wbkgd(screen->progress_window.w, COLOR_PAIR(COLOR_PROGRESSBAR));
      wbkgd(screen->status_window.w,   COLOR_PAIR(COLOR_STATUS));
      colors_use(screen->progress_window.w, COLOR_PROGRESSBAR);
    }

  /* initialize screens */
  screen->screen_list = NULL;
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_playlist());
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_browse());
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_help());
#ifdef ENABLE_KEYDEF_SCREEN
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_keydef());
#endif
#ifdef ENABLE_CLOCK_SCREEN
  screen->screen_list = g_list_append(screen->screen_list, 
				      (gpointer) get_screen_clock());
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
  if( mode_fn && mode_fn->open )
    mode_fn->open(screen, c);

  /* initialize wreadln */
  wrln_resize_callback = screen_resize;
  wrln_max_history_length = 16;

  return 0;
}

void 
screen_paint(mpdclient_t *c)
{
  /* paint the title/header window */
  if( mode_fn && mode_fn->get_title )
    paint_top_window(mode_fn->get_title(screen->buf,screen->buf_size), c, 1);
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
screen_update(mpdclient_t *c)
{
  static int repeat = -1;
  static int random = -1;
  static int crossfade = -1;
  static int dbupdate = -1;
  list_window_t *lw = NULL;

  if( !screen->painted )
    return screen_paint(c);

  /* print a message if mpd status has changed */
  if( repeat<0 )
    {
      repeat = c->status->repeat;
      random = c->status->random;
      crossfade = c->status->crossfade;
      dbupdate = c->status->updatingDb;
    }
  if( repeat != c->status->repeat )
    screen_status_printf(c->status->repeat ? 
			 _("Repeat is on") :
			 _("Repeat is off"));
  if( random != c->status->random )
    screen_status_printf(c->status->random ?
			 _("Random is on") :
			 _("Random is off"));
			 
  if( crossfade != c->status->crossfade )
    screen_status_printf(_("Crossfade %d seconds"), c->status->crossfade);
  if( dbupdate && dbupdate != c->status->updatingDb )
    {
      screen_status_printf(_("Database updated!"));
      mpdclient_browse_callback(c, BROWSE_DB_UPDATED, NULL);
    }

  repeat = c->status->repeat;
  random = c->status->random;
  crossfade = c->status->crossfade;
  dbupdate = c->status->updatingDb;

  /* update title/header window */
  if( welcome && screen->last_cmd==CMD_NONE &&
      time(NULL)-screen->start_timestamp <= SCREEN_WELCOME_TIME)
    paint_top_window("", c, 0);
  else if( mode_fn && mode_fn->get_title )
    {
      paint_top_window(mode_fn->get_title(screen->buf,screen->buf_size), c, 0);
      welcome = FALSE;
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
screen_idle(mpdclient_t *c)
{
  if( c->song && seek_id ==  c->song->id &&
      (screen->last_cmd == CMD_SEEK_FORWARD || 
       screen->last_cmd == CMD_SEEK_BACKWARD) )
    {
      mpdclient_cmd_seek(c, seek_id, seek_target_time);
    }

  screen->last_cmd = CMD_NONE;
  seek_id = -1;
}

void 
screen_cmd(mpdclient_t *c, command_t cmd)
{
  screen_mode_t new_mode = screen->mode;

  screen->input_timestamp = time(NULL);
  screen->last_cmd = cmd;
  welcome = FALSE;

  if( mode_fn && mode_fn->cmd && mode_fn->cmd(screen, c, cmd) )
    return;

  switch(cmd)
    {
    case CMD_PLAY:
      mpdclient_cmd_play(c, MPD_PLAY_AT_BEGINNING);
      break;
    case CMD_PAUSE:
      mpdclient_cmd_pause(c, !IS_PAUSED(c->status->state));
      break;
    case CMD_STOP:
      mpdclient_cmd_stop(c);
      break;
    case CMD_SEEK_FORWARD:
      if( !IS_STOPPED(c->status->state) )
	{
	  if( c->song && seek_id != c->song->id )
	    {
	      seek_id = c->song->id;
	      seek_target_time = c->status->elapsedTime;
	    }
	  seek_target_time++;
	  if( seek_target_time < c->status->totalTime )
	    break;
	  seek_target_time = c->status->totalTime;
	  /* seek_target_time=0; */
	}
      break;
      /* fall through... */
    case CMD_TRACK_NEXT:
      if( !IS_STOPPED(c->status->state) )
	mpdclient_cmd_next(c);
      break;
    case CMD_SEEK_BACKWARD:
      if( !IS_STOPPED(c->status->state) )
	{
	  if( seek_id != c->song->id )
	    {
	      seek_id = c->song->id;
	      seek_target_time = c->status->elapsedTime;
	    }
	  seek_target_time--;
	  if( seek_target_time < 0 )
	    seek_target_time=0;
	}
      break;
    case CMD_TRACK_PREVIOUS:
      if( !IS_STOPPED(c->status->state) )
	mpdclient_cmd_prev(c);
      break;   
    case CMD_SHUFFLE:
      if( mpdclient_cmd_shuffle(c) == 0 )
	screen_status_message(_("Shuffled playlist!"));
      break;
    case CMD_CLEAR:
      if( mpdclient_cmd_clear(c) == 0 )
	screen_status_message(_("Cleared playlist!"));
      break;
    case CMD_REPEAT:
      mpdclient_cmd_repeat(c, !c->status->repeat);
      break;
    case CMD_RANDOM:
      mpdclient_cmd_random(c, !c->status->random);
      break;
    case CMD_CROSSFADE:
      mpdclient_cmd_crossfade(c, c->status->crossfade ? 0 : CROSSFADE_TIME);
      break;
    case CMD_DB_UPDATE:
      if( !c->status->updatingDb )
	{
	  if( mpdclient_cmd_db_update(c,NULL)==0 )
	    screen_status_printf(_("Database update started!"));
	}
      else
	screen_status_printf(_("Database update running..."));
      break;
    case CMD_VOLUME_UP:
      if( c->status->volume!=MPD_STATUS_NO_VOLUME && c->status->volume<100 )
	mpdclient_cmd_volume(c, ++c->status->volume);
      break;
    case CMD_VOLUME_DOWN:
      if( c->status->volume!=MPD_STATUS_NO_VOLUME && c->status->volume>0 )
	mpdclient_cmd_volume(c, --c->status->volume);
      break;
    case CMD_TOGGLE_FIND_WRAP:
      options.find_wrap = !options.find_wrap;
      screen_status_printf(options.find_wrap ? 
			   _("Find mode: Wrapped") :
			   _("Find mode: Normal"));
      break;
    case CMD_TOGGLE_AUTOCENTER:
      options.auto_center = !options.auto_center;
      screen_status_printf(options.auto_center ?
			   _("Auto center mode: On") :
			   _("Auto center mode: Off"));
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
#ifdef ENABLE_CLOCK_SCREEN 
    case CMD_SCREEN_CLOCK:
      switch_screen_mode(SCREEN_CLOCK_WINDOW, c);
      break;
#endif
    case CMD_QUIT:
      exit(EXIT_SUCCESS);
    default:
      break;
    }
}



