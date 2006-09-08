

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#include "ncmpc.h"
#include "colors.h"


gpointer get_input(void *null)
{
  int key;
  while(key = getch())
  {
    if(key == 'q')
    {
      exit(0);
    }
  }
  return;
}

void draw_message(char *msg)
{
  int rows, cols;
  getmaxyx(stdscr, rows, cols);
  mvaddstr(rows-1, (cols/2)-(strlen(msg)/2), msg); 
  refresh();
}


/*void draw_title()
{
  colors_use(stdscr, COLOR_TITLE_BOLD);
  mvaddstr(rows/2+1, COLS/2, VERSION);
  colors_use(stdscr, COLOR_TITLE_BOLD);
  mvaddstr(rows/2-1, x, PACKAGE);
  refresh();
}
*/
gboolean advance_version()
{
  int rows, cols;
  getmaxyx(stdscr, rows, cols);
  static int x = 0;
  if(x == 0)
  {
    x = cols - strlen(VERSION);
  }
  colors_use(stdscr, COLOR_TITLE_BOLD);
  mvaddstr(rows/2+1, x--, VERSION);
  mvhline(rows/2+1, x+strlen(VERSION)+1, ' ', cols);
  refresh();
  if(x == cols/2) return FALSE;
  return TRUE;
}
     
      
gboolean advance_name()
{
  int rows, cols;
  getmaxyx(stdscr, rows, cols);
  static int x = 0;
  colors_use(stdscr, COLOR_TITLE_BOLD);
  mvaddstr(rows/2-1, x, PACKAGE);
  mvhline(rows/2-1, 0, ' ', x);
  refresh();
  if(x + strlen(PACKAGE)  == cols/2) return FALSE;
  x++;
  return TRUE;
}

gboolean draw_animation(gpointer *data)
{ //need this to execute both functions, even if one of them return TRUE
  if(advance_name() == FALSE && advance_version() == FALSE || advance_name() == TRUE && advance_version() == FALSE)
  {
    //  system("sleep 8");
    g_main_loop_quit((GMainLoop*) data);
    //    g_source_attach(((int*)data)[1], data);
    return FALSE;
  }

  return TRUE;
}

void drawx()
{
  //  g_thread_create(get_input, NULL, FALSE, NULL);
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  fprintf(stderr, "%d", rows/2);

  mvhline(rows/2, 0,  ACS_HLINE  , cols);
  draw_message("Connecting...");
  //advance_version(); 
  refresh();

  GMainContext *cont = g_main_context_new();
  GMainLoop *loop = g_main_loop_new(cont, FALSE);
 
  GSource *frame =  g_timeout_source_new(3);
  GSource *state = g_timeout_source_new(100);
  GSource *stopper = g_timeout_source_new(200);

  void *blubb = malloc(sizeof(GMainLoop*)+sizeof(GSource*));
  blubb = loop;
  ((int*)blubb)[1] = stopper;

  g_source_set_callback(frame, draw_animation ,blubb, NULL);  
  g_source_attach(frame, g_main_loop_get_context(loop));
 
  g_main_loop_run(loop);
}


void draw_splash()
{
  drawx();
}

//int draw_frame
