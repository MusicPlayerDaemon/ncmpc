/* 
 * $Id: screen_utils.c,v 1.4 2004/03/16 13:57:24 kalle Exp $ 
 *
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"

#if 0
#include <readline/readline.h>
#endif


list_window_t *
list_window_init(WINDOW *w, int width, int height)
{
  list_window_t *lw;

  lw = malloc(sizeof(list_window_t));
  memset(lw, 0, sizeof(list_window_t));
  lw->w = w;
  lw->cols = width;
  lw->rows = height;
  lw->clear = 1;
  return lw;
}

list_window_t *
list_window_free(list_window_t *lw)
{
  if( lw )
    {
      memset(lw, 0, sizeof(list_window_t));
      free(lw);
    }
  return NULL;
}

void
list_window_reset(list_window_t *lw)
{
  lw->selected = 0;
  lw->start = 0;
  lw->clear = 1;
}

void 
list_window_set_selected(list_window_t *lw, int n)
{
  lw->selected=n;
}

void 
list_window_paint(list_window_t *lw,
		  list_window_callback_fn_t callback,
		  void *callback_data)
{
  int i;

  while( lw->selected < lw->start )
    {
      lw->start--;
      lw->clear=1;
    }
  while( lw->selected >= lw->start+lw->rows )
    {
      lw->start++;
      lw->clear=1;
    }
  if( lw->clear )
    {
      wclear(lw->w);
      lw->clear=0;
    }

  for(i=0; i<lw->rows; i++)
    {
      int highlight;
      char *label;

      label = (callback) (lw->start+i, &highlight, callback_data);
      if( label )
	{
	  wmove(lw->w, i, 0);
	  if( highlight )
	    wattron(lw->w, A_BOLD);
	  if( lw->start+i == lw->selected )
	    wattron(lw->w, A_REVERSE);
	  
	  waddnstr(lw->w, label, lw->cols);

	  if( highlight )
	    wattroff(lw->w, A_BOLD);
	  if( lw->start+i == lw->selected )
	    wattroff(lw->w, A_REVERSE);
	}
    }
}

void
list_window_next(list_window_t *lw, int length)
{
  if( lw->selected < length-1 )
    lw->selected++;
}

void
list_window_previous(list_window_t *lw)
{
  if( lw->selected > 0 )
    lw->selected--;
}

void
list_window_first(list_window_t *lw)
{
  lw->selected = 0;
}

void
list_window_last(list_window_t *lw, int length)
{
  lw->selected = length-1;
}

void
list_window_next_page(list_window_t *lw, int length)
{
  int step = lw->rows-1;
  if( step<= 0 )
    return;
  if( lw->selected+step < length-1 )
    lw->selected+=step;
  else
    return list_window_last(lw,length);
}

void
list_window_previous_page(list_window_t *lw)
{
  int step = lw->rows-1;
  if( step<= 0 )
    return;
  if( lw->selected-step > 0 )
    lw->selected-=step;
  else
    list_window_first(lw);
}

int
list_window_find(list_window_t *lw, 
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 char *str)
{
  int i = lw->selected+1;

  while( i< lw->rows )
    {
      int h;
      char *label = (callback) (i,&h,callback_data);
      
      if( str && label && strstr(label, str) )
	{
	  lw->selected = i;
	  return 0;
	}
      i++;
    }
  return 1;
}


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

