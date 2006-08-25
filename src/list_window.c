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
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "options.h"
#include "support.h"
#include "command.h"
#include "colors.h"
#include "list_window.h"

extern void screen_bell(void);

list_window_t *
list_window_init(WINDOW *w, int width, int height)
{
  list_window_t *lw;

  lw = g_malloc0(sizeof(list_window_t));
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
  lw->xoffset = 0;
  lw->start = 0;
  lw->clear = 1;
}

void
list_window_check_selected(list_window_t *lw, int length)
{
  while( lw->start && lw->start+lw->rows>length)
    lw->start--;

  if( lw->selected<0 )
    lw->selected=0;

  while( lw->selected<lw->start )
    lw->selected++;

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
  else if ( options.list_wrap )
    lw->selected = 0;
}

void
list_window_previous(list_window_t *lw, int length)
{
  if( lw->selected > 0 )
    lw->selected--;
  else if( options.list_wrap )
    lw->selected = length-1;
}

void
list_window_right(list_window_t *lw, int length)
{
  lw->xoffset++;
}

void
list_window_left(list_window_t *lw)
{
  if( lw->xoffset > 0 )
    lw->xoffset--;
}

void
list_window_first(list_window_t *lw)
{
  lw->xoffset = 0;
  lw->selected = 0;
}

void
list_window_last(list_window_t *lw, int length)
{
  lw->xoffset = 0;
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
  int fill = options.wide_cursor;
  int show_cursor = !(lw->flags & LW_HIDE_CURSOR);

  if( show_cursor )
    {
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
    }
  
  for(i=0; i<lw->rows; i++)
    {
      int highlight = 0;
      char *label;

      label = (callback) (lw->start+i, &highlight, callback_data);
      wmove(lw->w, i, 0);
      if( lw->clear && (!fill || !label) )
	wclrtoeol(lw->w);
      if( label )
	{
	  int selected = lw->start+i == lw->selected;
	  size_t len = my_strlen(label);

	  if( highlight )
	    colors_use(lw->w, COLOR_LIST_BOLD);
	  else
	    colors_use(lw->w, COLOR_LIST);

	  if( show_cursor && selected )
	    wattron(lw->w, A_REVERSE);
	  
	  //waddnstr(lw->w, label, lw->cols);
	  waddstr(lw->w, label);
	   if( fill && len<lw->cols )
	    whline(lw->w,  ' ', lw->cols-len);

	  if( selected )
	    wattroff(lw->w, A_REVERSE);	  
	}
	
    }
  lw->clear=0;
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
	  if ( i==0 ) /* empty list */
	    return 1;
	  i=0; /* first item */
	  screen_bell();
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

  if ( rows == 0 )
    return 1;

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
	  screen_bell();
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
      list_window_previous(lw, rows);
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





list_window_state_t *
list_window_init_state(void)
{
  return g_malloc0(sizeof(list_window_state_t));
}

list_window_state_t *
list_window_free_state(list_window_state_t *state)
{
  if( state )
    {
      if( state->list )
	{
	  GList *list = state->list;
	  while( list )
	    {
	      g_free(list->data);
	      list->data = NULL;
	      list = list->next;
	    }
	  g_list_free(state->list);
	  state->list = NULL;
	}
      g_free(state);
    }
  return NULL;
}

void 
list_window_push_state(list_window_state_t *state, list_window_t *lw)
{
  if( state )
    {
      list_window_t *tmp = g_malloc(sizeof(list_window_t));
      memcpy(tmp, lw, sizeof(list_window_t));
      state->list = g_list_prepend(state->list, (gpointer) tmp);
      list_window_reset(lw);
    }
}

void 
list_window_pop_state(list_window_state_t *state, list_window_t *lw)
{
  if( state && state->list )
    {
      list_window_t *tmp = state->list->data;

      memcpy(lw, tmp, sizeof(list_window_t));
      g_free(tmp);
      state->list->data = NULL;
      state->list = g_list_delete_link(state->list, state->list);
    }
}



