#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "libmpdclient.h"
#include "mpc.h"
#include "support.h"
#include "command.h"
#include "options.h"
#include "screen.h"

char *
screen_readln(WINDOW *w, char *prompt)
{
  char buf[256], *line = NULL;
  int prompt_len = strlen(prompt);

  wclear(w);  
  wmove(w, 0, 0);
  waddstr(w, prompt);
  wmove(w, 0, prompt_len);
  
  echo();
  curs_set(1);

  if( wgetnstr(w, buf, 256) == OK )
    line = strdup(buf);

  noecho();
  curs_set(0);

  return line;
}

int
my_waddstr(WINDOW *w, const char *text, int color)
{
  int ret;

  if( options.enable_colors )
    wattron(w, color);
  ret = waddstr(w, text);
  if( options.enable_colors )
    wattroff(w, color);

  return ret;
}

int
my_mvwaddstr(WINDOW *w, int x, int y, const char *text, int color)
{
  int ret;

  if( options.enable_colors )
    wattron(w, color);
  ret = mvwaddstr(w, x, y, text);
  if( options.enable_colors )
    wattroff(w, color);

  return ret;
}
