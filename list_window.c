#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "support.h"
#include "command.h"
#include "list_window.h"

list_window_t *
list_window_init(WINDOW *w, int width, int height)
{
  list_window_t *lw;

  lw = g_malloc(sizeof(list_window_t));
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
      g_free(lw);
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
list_window_check_selected(list_window_t *lw, int length)
{
  if( lw->selected<0 )
    lw->selected=0;

  while( lw->selected>0 && length>0 && lw->selected>=length )
    lw->selected--;
}

void 
list_window_set_selected(list_window_t *lw, int n)
{
  lw->selected=n;
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


int
list_window_find(list_window_t *lw, 
		 list_window_callback_fn_t callback,
		 void *callback_data,
		 char *str,
		 int wrap)
{
  int h;
  int i = lw->selected+1;
  char *label;
  
  while( wrap || i==lw->selected+1 )
    {
      while( (label=(callback) (i,&h,callback_data)) )
	{
	  if( str && label && strcasestr(label, str) )
	    {
	      lw->selected = i;
	      return 0;
	    }
	  if( wrap && i==lw->selected )
	    return 1;
	  i++;
	}
      if( wrap )
	{
	  i=0; /* first item */
	  beep(); 
	}
    }
  return 1;
}


int
list_window_rfind(list_window_t *lw, 
		  list_window_callback_fn_t callback,
		  void *callback_data,
		  char *str,
		  int wrap,
		  int rows)
{
  int h;
  int i = lw->selected-1;
  char *label;

  while( wrap || i==lw->selected-1 )
    {
      while( i>=0 && (label=(callback) (i,&h,callback_data)) )
	{
	  if( str && label && strcasestr(label, str) )
	    {
	      lw->selected = i;
	      return 0;
	    }
	  if( wrap && i==lw->selected )
	    return 1;
	  i--;
	}
      if( wrap )
	{
	  i=rows-1; /* last item */
	  beep();
	}
    }
  return 1;
}


/* perform basic list window commands (movement) */
int 
list_window_cmd(list_window_t *lw, int rows, command_t cmd)
{
  switch(cmd)
    {
    case CMD_LIST_PREVIOUS:
      list_window_previous(lw);
      lw->repaint=1;
      break;
    case CMD_LIST_NEXT:
      list_window_next(lw, rows);
      lw->repaint=1;
      break;
    case CMD_LIST_FIRST:
      list_window_first(lw);
      lw->repaint  = 1;
      break;
    case CMD_LIST_LAST:
      list_window_last(lw, rows);
      lw->repaint = 1;
      break;
    case CMD_LIST_NEXT_PAGE:
      list_window_next_page(lw, rows);
      lw->repaint  = 1;
      break;
    case CMD_LIST_PREVIOUS_PAGE:
      list_window_previous_page(lw);
      lw->repaint  = 1;
      break;
    default:
      return 0;
    }
  return 1;
}


