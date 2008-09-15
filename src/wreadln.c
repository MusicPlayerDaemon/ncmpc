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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#ifdef USE_NCURSESW
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#include "wreadln.h"

#define KEY_CTRL_A   1
#define KEY_CTRL_C   3
#define KEY_CTRL_D   4 
#define KEY_CTRL_E   5
#define KEY_CTRL_G   7
#define KEY_CTRL_K   11
#define KEY_CTRL_U   21
#define KEY_CTRL_Z   26
#define KEY_BCKSPC   8
#define TAB          9

#define WRLN_MAX_LINE_SIZE 1024
#define WRLN_MAX_HISTORY_LENGTH 32
 
guint wrln_max_line_size = WRLN_MAX_LINE_SIZE;
guint wrln_max_history_length = WRLN_MAX_HISTORY_LENGTH;
wrln_wgetch_fn_t wrln_wgetch = NULL;
void *wrln_completion_callback_data = NULL;
wrln_gcmp_pre_cb_t wrln_pre_completion_callback = NULL;
wrln_gcmp_post_cb_t wrln_post_completion_callback = NULL;

extern void sigstop(void);
extern void screen_bell(void);
extern size_t my_strlen(char *str);

#ifndef USE_NCURSESW
/* move the cursor one step to the right */
static inline void cursor_move_right(gint *cursor,
				     gint *start,
				     gint width,
				     gint x0,
				     gint x1,
				     gchar *line)
{
	if (*cursor < strlen(line) && *cursor < wrln_max_line_size - 1) {
		(*cursor)++;
		if (*cursor + x0 >= x1 && *start < *cursor - width + 1)
			(*start)++;
	}
}

/* move the cursor one step to the left */
static inline void cursor_move_left(gint *cursor,
                                    gint *start)
{
  if( *cursor > 0 )
    {
      if( *cursor==*start && *start > 0 )
	(*start)--;
      (*cursor)--;
    }
}

/* move the cursor to the end of the line */
static inline void cursor_move_to_eol(gint *cursor,
				      gint *start,
				      gint width,
				      gint x0,
				      gint x1,
				      gchar *line)
{
  *cursor = strlen(line);
  if( *cursor+x0 >= x1 )
    *start = *cursor-width+1;
}

/* draw line buffer and update cursor position */
static inline void drawline(gint cursor,
			    gint start,
			    gint width,
			    gint x0,
			    gint y,
			    gboolean masked,
			    gchar *line,
			    WINDOW *w)
{
  wmove(w, y, x0);
  /* clear input area */
  whline(w, ' ', width);
  /* print visible part of the line buffer */
  if(masked == TRUE) whline(w, '*', my_strlen(line)-start);
  else waddnstr(w, line+start, width);
  /* move the cursor to the correct position */
  wmove(w, y, x0 + cursor-start);
  /* tell ncurses to redraw the screen */
  doupdate();
}


/* libcurses version */

gchar *
_wreadln(WINDOW *w,
	 gchar *prompt,
	 gchar *initial_value,
	 gint x1,
	 GList **history,
	 GCompletion *gcmp,
	 gboolean masked)
{
	GList *hlist = NULL, *hcurrent = NULL;
	gchar *line;
	gint x0, y, width;
	gint cursor = 0, start = 0;
	gint key = 0, i;

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

	if( history ) {
		/* append the a new line to our history list */
		*history = g_list_append(*history, g_malloc0(wrln_max_line_size));
		/* hlist points to the current item in the history list */
		hlist =  g_list_last(*history);
		hcurrent = hlist;
	}

	if( initial_value == (char *) -1 ) {
		/* get previous history entry */
		if( history && hlist->prev )
			{
				if( hlist==hcurrent )
					{
						/* save the current line */
						g_strlcpy(hlist->data, line, wrln_max_line_size);
					}
				/* get previous line */
				hlist = hlist->prev;
				g_strlcpy(line, hlist->data, wrln_max_line_size);
			}
		cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
		drawline(cursor, start, width, x0, y, masked, line, w);
	} else if( initial_value ) {
		/* copy the initial value to the line buffer */
		g_strlcpy(line, initial_value, wrln_max_line_size);
		cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
		drawline(cursor, start, width, x0, y, masked, line, w);
	}

	while( key!=13 && key!='\n' ) {
		if( wrln_wgetch )
			key = wrln_wgetch(w);
		else
			key = wgetch(w);

		/* check if key is a function key */
		for(i=0; i<63; i++)
			if( key==KEY_F(i) ) {
				key=KEY_F(1);
				i=64;
			}

		switch (key) {
#ifdef HAVE_GETMOUSE
		case KEY_MOUSE: /* ignore mouse events */
#endif
		case ERR: /* ingnore errors */
			break;

		case KEY_RESIZE:
			/* a resize event */
			if( x1>COLS ) {
				x1=COLS;
				width = x1-x0;
				cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
			}
			/* make shure the cursor is visible */
			curs_set(1);
			break;

		case TAB:
			if( gcmp ) {
				char *prefix = NULL;
				GList *list;

				if(wrln_pre_completion_callback)
					wrln_pre_completion_callback(gcmp, line,
								     wrln_completion_callback_data);
				list = g_completion_complete(gcmp, line, &prefix);
				if( prefix ) {
					g_strlcpy(line, prefix, wrln_max_line_size);
					cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
					g_free(prefix);
				}
				else
					screen_bell();
				if( wrln_post_completion_callback )
					wrln_post_completion_callback(gcmp, line, list,
								      wrln_completion_callback_data);
			}
			break;

		case KEY_CTRL_G:
			screen_bell();
			g_free(line);
			if( history ) {
				g_free(hcurrent->data);
				hcurrent->data = NULL;
				*history = g_list_delete_link(*history, hcurrent);
			}
			return NULL;

		case KEY_LEFT:
			cursor_move_left(&cursor, &start);
			break;
		case KEY_RIGHT:
			cursor_move_right(&cursor, &start, width, x0, x1, line);
			break;
		case KEY_HOME:
		case KEY_CTRL_A:
			cursor = 0;
			start = 0;
			break;
		case KEY_END:
		case KEY_CTRL_E:
			cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
			break;
		case KEY_CTRL_K:
			line[cursor] = 0;
			break;
		case KEY_CTRL_U:
			cursor = my_strlen(line);
			for (i = 0;i < cursor; i++)
				line[i] = '\0';
			cursor = 0;
			break;
		case 127:
		case KEY_BCKSPC:	/* handle backspace: copy all */
		case KEY_BACKSPACE:	/* chars starting from curpos */
			if( cursor > 0 ) {/* - 1 from buf[n+1] to buf   */
				for (i = cursor - 1; line[i] != 0; i++)
					line[i] = line[i + 1];
				cursor_move_left(&cursor, &start);
			}
			break;
		case KEY_DC:		/* handle delete key. As above */
		case KEY_CTRL_D:
			if (cursor <= my_strlen(line) - 1) {
				for (i = cursor; line[i] != 0; i++)
					line[i] = line[i + 1];
			}
			break;
		case KEY_UP:
			/* get previous history entry */
			if( history && hlist->prev ) {
				if( hlist==hcurrent )
					{
						/* save the current line */
						g_strlcpy(hlist->data, line, wrln_max_line_size);
					}
				/* get previous line */
				hlist = hlist->prev;
				g_strlcpy(line, hlist->data, wrln_max_line_size);
			}
			cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
			break;
		case KEY_DOWN:
			/* get next history entry */
			if( history && hlist->next ) {
				/* get next line */
				hlist = hlist->next;
				g_strlcpy(line, hlist->data, wrln_max_line_size);
			}
			cursor_move_to_eol(&cursor, &start, width, x0, x1, line);
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
			if (key >= 32) {
				if (strlen (line + cursor)) { /* if the cursor is */
					/* not at the last pos */
					gchar *tmp = 0;
					gsize size = strlen(line + cursor) + 1;

					tmp = g_malloc0(size);
					g_strlcpy (tmp, line + cursor, size);
					line[cursor] = key;
					line[cursor + 1] = 0;
					g_strlcat (&line[cursor + 1], tmp, size);
					g_free(tmp);
					cursor_move_right(&cursor, &start, width, x0, x1, line);
				} else {
					line[cursor + 1] = 0;
					line[cursor] = key;
					cursor_move_right(&cursor, &start, width, x0, x1, line);
				}
			}
		}

		drawline(cursor, start, width, x0, y, masked, line, w);
	}

	/* update history */
	if( history ) {
		if( strlen(line) ) {
			/* update the current history entry */
			size_t size = strlen(line)+1;
			hcurrent->data = g_realloc(hcurrent->data, size);
			g_strlcpy(hcurrent->data, line, size);
		} else {
			/* the line was empty - remove the current history entry */
			g_free(hcurrent->data);
			hcurrent->data = NULL;
			*history = g_list_delete_link(*history, hcurrent);
		}

		while( g_list_length(*history) > wrln_max_history_length ) {
			GList *first = g_list_first(*history);

			/* remove the oldest history entry  */
			g_free(first->data);
			first->data = NULL;
			*history = g_list_delete_link(*history, first);
		}
	}

	return g_realloc(line, strlen(line)+1);
}

#else

/* move the cursor one step to the right */
static inline void cursor_move_right(gint *cursor,
				     gint *start,
				     gint width,
				     gint x0,
				     gint x1,
				     wchar_t *wline)
{
  if( *cursor < wcslen(wline) && *cursor<wrln_max_line_size-1 )
    {
      (*cursor)++;
      if( *cursor+x0 >= x1 && *start<*cursor-width+1)
	(*start)++;
    }
}

/* move the cursor one step to the left */
static inline void cursor_move_left(gint *cursor,
				    gint *start,
				    gint width,
				    gint x0,
				    gint x1,
				    wchar_t *line)
{
  if( *cursor > 0 )
    {
      if( *cursor==*start && *start > 0 )
	(*start)--;
      (*cursor)--;
    }
}


static inline void backspace(gint *cursor,
			     gint *start,
			     gint width,
			     gint x0,
			     gint x1,
			     wchar_t *wline) 
{
  int i;
  if( *cursor > 0 )    
    {
      for (i = *cursor - 1; wline[i] != 0; i++)
	wline[i] = wline[i + 1];
      cursor_move_left(cursor, start, width, x0, x1, wline);
    }
}

/* handle delete */
static inline void delete(gint *cursor,
			  wchar_t *wline) 
{
  int i;
  if( *cursor <= wcslen(wline) - 1 ) 
    {
      for (i = *cursor; wline[i] != 0; i++)
	wline[i] = wline[i + 1];
    }
}

/* move the cursor to the end of the line */
static inline void cursor_move_to_eol(gint *cursor,
				      gint *start,
				      gint width,
				      gint x0,
				      gint x1,
				      wchar_t *line)
{
  *cursor = wcslen(line);
  if( *cursor+x0 >= x1 )
    *start = *cursor-width+1;
}

/* draw line buffer and update cursor position */
static inline void drawline(gint cursor,
			    gint start,
			    gint width,
			    gint x0,
			    gint y,
			    gboolean masked,
			    wchar_t *line,
			    WINDOW *w)
{
  wmove(w, y, x0);
  /* clear input area */
  whline(w, ' ', width);
  /* print visible part of the line buffer */
  if(masked == TRUE) whline(w, '*', wcslen(line)-start);
  else waddnwstr(w, line+start, width);
  FILE *dbg = fopen ("dbg", "a+");
  fprintf (dbg, "%i,%s---%i---", width, line, wcslen (line));
  /* move the cursor to the correct position */
  wmove(w, y, x0 + cursor-start);
  /* tell ncurses to redraw the screen */
  doupdate();
}

/* libcursesw version */ 

gchar *
_wreadln(WINDOW *w,
	 gchar *prompt,
	 gchar *initial_value,
	 gint x1,
	 GList **history,
	 GCompletion *gcmp,
	 gboolean masked)
{
	GList *hlist = NULL, *hcurrent = NULL;
	wchar_t *wline;
	gchar *mbline;
	gint x0, x, y, width, start;
	gint cursor;
	wint_t wch;
	gint key;
	gint i;

  /* initialize variables */
  start = 0;
  x = 0;
  cursor = 0;
  mbline = NULL;

  /* allocate a line buffer */
  wline = g_malloc0(wrln_max_line_size*sizeof(wchar_t));
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
	      //g_strlcpy(hlist->data, line, wrln_max_line_size);
	    }
	  /* get previous line */
	  hlist = hlist->prev;
	  mbstowcs(wline, hlist->data, wrln_max_line_size);
	}
      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
      drawline(cursor, start, width, x0, y, masked, wline, w);
    }
  else if( initial_value )
    {
      /* copy the initial value to the line buffer */
      mbstowcs(wline, initial_value, wrln_max_line_size);
      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
      drawline(cursor, start, width, x0, y, masked, wline, w);
    }  

  wch=0;
  key=0;
  while( wch!=13 && wch!='\n' )
    {
      key = wget_wch(w, &wch);

      if( key==KEY_CODE_YES )
	{
	  /* function key */
	  switch(wch)
	    {
	    case KEY_HOME:
	      x=0;
	      cursor=0;
	      start=0;
	      break;
	    case KEY_END:
	      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
	      break;
	    case KEY_LEFT:
	      cursor_move_left(&cursor, &start, width, x0, x1, wline);
	      break;
	    case KEY_RIGHT:
	      cursor_move_right(&cursor, &start, width, x0, x1, wline);
	      break;
	    case KEY_DC:
	      delete(&cursor, wline);
	      break;
	    case KEY_BCKSPC:
	    case KEY_BACKSPACE:	
	      backspace(&cursor, &start, width, x0, x1, wline);
	      break;
	    case KEY_UP:		
	      /* get previous history entry */
	      if( history && hlist->prev )
		{
		  if( hlist==hcurrent )
		    {
		      /* save the current line */
		      wcstombs(hlist->data, wline, wrln_max_line_size);
		    }
		  /* get previous line */
		  hlist = hlist->prev;
		  mbstowcs(wline, hlist->data, wrln_max_line_size);
		}
	      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
	      break; 
	    case KEY_DOWN:	
	      /* get next history entry */
	      if( history && hlist->next )
		{
		  /* get next line */
		  hlist = hlist->next;
		  mbstowcs(wline, hlist->data, wrln_max_line_size);
		}
	      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
	      break;
	    case KEY_RESIZE:
	      /* resize event */
	      if( x1>COLS )
		{
		  x1=COLS;
		  width = x1-x0;
		  cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
		}
	      /* make shure the cursor is visible */
	      curs_set(1);
	      break;
	    }

	}
      else if( key!=ERR )
	{
	  switch(wch)
	    {
	    case KEY_CTRL_A:
	      x=0;
	      cursor=0;
	      start=0;
	      break;
	    case KEY_CTRL_C:
	      exit(EXIT_SUCCESS);
	      break;
	    case KEY_CTRL_D:
	      delete(&cursor, wline);
	      break;
	    case KEY_CTRL_E:
	      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
	      break;
	    case TAB:
	      if( gcmp )
		{
		  char *prefix = NULL;
		  GList *list;
		  
		  i = wcstombs(NULL,wline,0)+1;
		  mbline = g_malloc0(i);
		  wcstombs(mbline, wline, i);
		  
		  if(wrln_pre_completion_callback)
		    wrln_pre_completion_callback(gcmp, mbline, 
						 wrln_completion_callback_data);
		  list = g_completion_complete(gcmp, mbline, &prefix);	      
		  if( prefix )
		    {
		      mbstowcs(wline, prefix, wrln_max_line_size);
		      cursor_move_to_eol(&cursor, &start, width, x0, x1, wline);
		      g_free(prefix);
		    }
		  else
		    screen_bell();
		  if( wrln_post_completion_callback )
		    wrln_post_completion_callback(gcmp, mbline, list,
						  wrln_completion_callback_data);
		  
		  g_free(mbline);
		}
	      break;
	    case KEY_CTRL_G:
	      screen_bell();
	      g_free(wline);
	      if( history )
		{
		  g_free(hcurrent->data);
		  hcurrent->data = NULL;
		  *history = g_list_delete_link(*history, hcurrent);
		}
	      return NULL;
	    case KEY_CTRL_K:
	      wline[cursor] = 0;
	      break;
	    case KEY_CTRL_U:
	      cursor = wcslen(wline);
	      for (i = 0;i < cursor; i++)
		wline[i] = '\0';
	      cursor = 0;
	      break;
	    case KEY_CTRL_Z:
	      sigstop();
	      break;
	    case 127:
	      backspace(&cursor, &start, width, x0, x1, wline);
	      break;
	    case '\n':
	    case 13:
	      /* ignore char */
	      break;
	    default:
	      if( (wcslen(wline+cursor)) )
		{
		  /* the cursor is not at the last pos */
		  wchar_t *tmp = NULL;
		  gsize len = (wcslen(wline+cursor)+1);
		  tmp = g_malloc0(len*sizeof(wchar_t));
		  wmemcpy(tmp, wline+cursor, len);
		  wline[cursor] = wch;
		  wline[cursor+1] = 0;
		  wcscat(&wline[cursor+1], tmp);
		  g_free(tmp);
		  cursor_move_right(&cursor, &start, width, x0, x1, wline);
		}
	      else
		{
		  FILE *ff = fopen ("curspr", "a+");
		  fprintf (ff, "%i", cursor);
		  wline[cursor] = wch;
		  wline[cursor+1] = 0;
		  cursor_move_right(&cursor, &start, width, x0, x1, wline);
		}
	    }
	}
      drawline(cursor, start, width, x0, y, masked, wline, w);
    }
  i = wcstombs(NULL,wline,0)+1;
  mbline = g_malloc0(i);
  wcstombs(mbline, wline, i);

  /* update history */
  if( history )
    {
      if( strlen(mbline) )
	{
	  /* update the current history entry */
	  size_t size = strlen(mbline)+1;
	  hcurrent->data = g_realloc(hcurrent->data, size);
	  g_strlcpy(hcurrent->data, mbline, size);
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
  return mbline;
}
 
#endif

gchar *
wreadln(WINDOW *w,
	gchar *prompt,
	gchar *initial_value,
	gint x1,
	GList **history,
	GCompletion *gcmp)
{
	return  _wreadln(w, prompt, initial_value, x1, history, gcmp, FALSE);
}

gchar *
wreadln_masked(WINDOW *w,
	       gchar *prompt,
	       gchar *initial_value,
	       gint x1,
	       GList **history,
	       GCompletion *gcmp)
{
	return  _wreadln(w, prompt, initial_value, x1, history, gcmp, TRUE);
}
