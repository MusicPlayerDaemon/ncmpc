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
#include <string.h>
#include <ncurses.h>
#include <glib.h>

#include "wreadln.h"

#define KEY_CTRL_A   1
#define KEY_CTRL_D   4 
#define KEY_CTRL_E   5
#define KEY_CTRL_G   7
#define KEY_CTRL_K   11
#define KEY_BCKSPC   8
#define TAB          9

#define WRLN_MAX_LINE_SIZE 1024
#define WRLN_MAX_HISTORY_LENGTH 32
 
unsigned int wrln_max_line_size = WRLN_MAX_LINE_SIZE;
unsigned int wrln_max_history_length = WRLN_MAX_HISTORY_LENGTH;
GVoidFunc wrln_resize_callback = NULL;
wrln_gcmp_pre_cb_t wrln_pre_completion_callback = NULL;
wrln_gcmp_post_cb_t wrln_post_completion_callback = NULL;


char *
wreadln(WINDOW *w, 
	char *prompt, 
	char *initial_value,
	int x1, 
	GList **history, 
	GCompletion *gcmp)
{
  GList *hlist = NULL, *hcurrent = NULL;
  char *line;
  int x0, y, width;		
  int cursor = 0, start = 0;		
  int key = 0, i;

  /* move the cursor one step to the right */
  void cursor_move_right(void) {
    if( cursor < strlen(line) && cursor<wrln_max_line_size-1 )
      {
	cursor++;
	if( cursor+x0 >= x1 && start<cursor-width+1)
	  start++;
      }
  }
  /* move the cursor one step to the left */
  void cursor_move_left(void) {
    if( cursor > 0 )
      {
	if( cursor==start && start > 0 )
	  start--;
	cursor--;
      }
  }
 /* move the cursor to the end of the line */
  void cursor_move_to_eol(void) {
    cursor = strlen(line);
    if( cursor+x0 >= x1 )
      start = cursor-width+1;
  }
  /* draw line buffer and update cursor position */
  void drawline() {
    wmove(w, y, x0);
    /* clear input area */
    whline(w, ' ', width);
    /* print visible part of the line buffer */
    waddnstr(w, line+start, width);
    /* move the cursor to the correct position */
    wmove(w, y, x0 + cursor-start);
    /* tell ncurses to redraw the screen */
    doupdate();
  }


  /* allocate a line buffer */
  line = g_malloc0(wrln_max_line_size);
  /* turn off echo */
  noecho();		
  /* make shure the cursor is visible */
  curs_set(1);
  /* print prompt string */
  if( prompt )
    waddstr(w, prompt);		
  /* retrive y and x0 position */
  getyx(w, y, x0);	
  /* check the x1 value */
  if( x1<=x0 || x1>COLS )
    x1 = COLS;
  width = x1-x0;
  /* clear input area */
  mvwhline(w, y, x0, ' ', width);	

  if( history )
    {
      /* append the a new line to our history list */
      *history = g_list_append(*history, g_malloc0(wrln_max_line_size));
      /* hlist points to the current item in the history list */
      hlist =  g_list_last(*history);
      hcurrent = hlist;
    }

  if( initial_value == (char *) -1 )
    {
      /* get previous history entry */
      if( history && hlist->prev )
	{
	  if( hlist==hcurrent )
	    {
	      /* save the current line */
	      strncpy(hlist->data, line, wrln_max_line_size);
	    }
	  /* get previous line */
	  hlist = hlist->prev;
	  strncpy(line, hlist->data, wrln_max_line_size);
	}
      cursor_move_to_eol();
      drawline();
    }
  else if( initial_value )
    {
      /* copy the initial value to the line buffer */
      strncpy(line, initial_value, wrln_max_line_size);
      cursor_move_to_eol();
      drawline();
    }  

  while( key!=13 && key!='\n' )
    {
      key = wgetch(w);

      /* check if key is a function key */
      for(i=0; i<63; i++)
	if( key==KEY_F(i) )
	  {
	    key=KEY_F(1);
	    i=64;
	  }

      switch (key)
	{
	case ERR:
	  /* ingnore errors */
	  break;

	case KEY_RESIZE:
	  /* a resize event -> call an external callback function */
	  if( wrln_resize_callback )
	    wrln_resize_callback();
	  if( x1>COLS )
	    {
	      x1=COLS;
	      width = x1-x0;
	      cursor_move_to_eol();
	    }
	  /* make shure the cursor is visible */
	  curs_set(1);
	  break;

	case TAB:
	  if( gcmp )
	    {
	      char *prefix = NULL;
	      GList *list;
	      
	      if(wrln_pre_completion_callback)
		wrln_pre_completion_callback(gcmp, line);
	      list = g_completion_complete(gcmp, line, &prefix);	      
	      if( prefix )
		{
		  int len = strlen(prefix);
		  strncpy(line, prefix, len);
		  cursor_move_to_eol();
		  g_free(prefix);
		}
	      else
		beep();
	      if( wrln_post_completion_callback )
		wrln_post_completion_callback(gcmp, line, list);
	    }
	  break;

	case KEY_CTRL_G:
	  beep();
	  g_free(line);
	  if( history )
	    {
	      g_free(hcurrent->data);
	      hcurrent->data = NULL;
	      *history = g_list_delete_link(*history, hcurrent);
	    }
	  return NULL;
	  
	case KEY_LEFT:
	  cursor_move_left();
	  break;
	case KEY_RIGHT:	
	  cursor_move_right();
	  break;
	case KEY_HOME:
	case KEY_CTRL_A:
	  cursor = 0;
	  start = 0;
	  break;
	case KEY_END:
	case KEY_CTRL_E:
	  cursor_move_to_eol();
	  break;
	case KEY_CTRL_K:
	  line[cursor] = 0;
	  break;
	case 127:
	case KEY_BCKSPC:	/* handle backspace: copy all */
	case KEY_BACKSPACE:	/* chars starting from curpos */
	  if( cursor > 0 )	/* - 1 from buf[n+1] to buf   */
	    {
	      for (i = cursor - 1; line[i] != 0; i++)
		line[i] = line[i + 1];
	      cursor_move_left();
	    }
	  break;
	case KEY_DC:		/* handle delete key. As above */
	case KEY_CTRL_D:
	  if( cursor <= strlen(line) - 1 ) 
	    {
	      for (i = cursor; line[i] != 0; i++)
		line[i] = line[i + 1];
	    }
	  break;
	case KEY_UP:		
	  /* get previous history entry */
	  if( history && hlist->prev )
	    {
	      if( hlist==hcurrent )
		{
		  /* save the current line */
		  strncpy(hlist->data, line, wrln_max_line_size);
		}
	      /* get previous line */
	      hlist = hlist->prev;
	      strncpy(line, hlist->data, wrln_max_line_size);
	    }
	  //	  if (cursor > strlen(line))
	  cursor_move_to_eol();
	  break;
	case KEY_DOWN:	
	  /* get next history entry */
	  if( history && hlist->next )
	    {
	      /* get next line */
	      hlist = hlist->next;
	      strncpy(line, hlist->data, wrln_max_line_size);
	    }
	  cursor_move_to_eol();
	  break;
	  
	case '\n':
	case 13:
	case KEY_IC:
	case KEY_PPAGE:
	case KEY_NPAGE:
	case KEY_F(1):
	  /* ignore char */
	  break;
	default:	 
	  if (key >= 32)
	    {
	      if (strlen (line + cursor))	/* if the cursor is */
		{		                /* not at the last pos */
		  char *tmp = 0;
		  tmp = g_malloc0(strlen (line + cursor) + 1);
		  strcpy (tmp, line + cursor);
		  line[cursor] = key;
		  line[cursor + 1] = 0;
		  strcat (&line[cursor + 1], tmp);
		  g_free(tmp);
		  cursor_move_right();
		}
	      else
		{
		  line[cursor + 1] = 0;
		  line[cursor] = key;
		  cursor_move_right();
		}
	    }
	}

      drawline();
    }

  /* update history */
  if( history )
    {
      if( strlen(line) )
	{
	  /* update the current history entry */
	  size_t size = strlen(line)+1;
	  hcurrent->data = g_realloc(hcurrent->data, size);
	  strncpy(hcurrent->data, line, size);
	}
      else
	{
	  /* the line was empty - remove the current history entry */
	  g_free(hcurrent->data);
	  hcurrent->data = NULL;
	  *history = g_list_delete_link(*history, hcurrent);
	}

      while( g_list_length(*history) > wrln_max_history_length )
	{
	  GList *first = g_list_first(*history);

	  /* remove the oldest history entry  */
	  g_free(first->data);
	  first->data = NULL;
	  *history = g_list_delete_link(*history, first);
	}
    }
  
  return g_realloc(line, strlen(line)+1);
}
 

