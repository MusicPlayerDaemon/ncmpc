/* 
 * $Id: screen_lyrics.c 3355 2006-09-1 17:44:04Z tradiaz $
 *	
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
 * Tue Aug  1 23:17:38 2006
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>
#include <expat.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <stdio.h>

#include "config.h"
#ifndef DISABLE_LYRICS_SCREEN
#include <sys/stat.h>
#include "ncmpc.h"
#include "options.h"
#include "mpdclient.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "easy_download.h"
#include "strfsong.h"
#include "src_lyrics.h"

int src_selection;

static void lyrics_paint(screen_t *screen, mpdclient_t *c);

FILE *create_lyr_file(char *artist, char *title)
{
                char path[1024];

                snprintf(path, 1024, "%s/.lyrics", 
                        getenv("HOME"));
                if(g_access(path, W_OK) != 0) if(mkdir(path, S_IRWXU) != 0) return NULL;
        
                snprintf(path, 1024, "%s/.lyrics/%s", 
                                getenv("HOME"), artist);
                if(g_access(path, W_OK) != 0) if(mkdir(path, S_IRWXU) != 0) return NULL;	
        
                snprintf(path, 1024, "%s/.lyrics/%s/%s.lyric", 
                                getenv("HOME"), artist, title);

            return fopen(path, "w");
}	

    
int store_lyr_hd()
{
        char artist[512];
        char title[512];
        static char path[1024];
        FILE *lyr_file;
        
        get_text_line(&lyr_text, 0, artist, 512);
        get_text_line(&lyr_text, 1, title, 512);
        artist[strlen(artist)-1] = '\0';
        title[strlen(title)-1] = '\0';
        
        snprintf(path, 1024, "%s/.lyrics/%s/%s.lyric", 
                        getenv("HOME"), artist, title);
        lyr_file = create_lyr_file(artist, title);
        if(lyr_file == NULL) return -1;
        int i;
        char line_buf[1024];
        
        for(i = 3; i <= lyr_text.text->len; i++)
        {
                if(get_text_line(&lyr_text, i, line_buf, 1024) == -1);
                fputs(line_buf, lyr_file);
        }
        fclose(lyr_file);
        return 0;
}
                                
        
void check_repaint()
{
        if(screen_get_id("lyrics") == get_cur_mode_id())lyrics_paint(NULL, NULL);
}


gpointer get_lyr(void *c)
{
        mpd_Status *status = ((retrieval_spec*)c)->client->status;
        mpd_Song *cur = ((retrieval_spec*)c)->client->song; 
        //mpdclient_update((mpdclient_t*)c);
        
        if(!(IS_PAUSED(status->state)||IS_PLAYING(status->state)))
        {
                formed_text_init(&lyr_text);			
                return NULL;
        }
        

        char artist[MAX_SONGNAME_LENGTH];
        char title[MAX_SONGNAME_LENGTH];
        lock = 2;
        result = 0;
        
        formed_text_init(&lyr_text);
        
        strfsong(artist, MAX_SONGNAME_LENGTH, "%artist%", cur);
        strfsong(title, MAX_SONGNAME_LENGTH, "%title%", cur);
        
        //write header..
        formed_text_init(&lyr_text);
        add_text_line(&lyr_text, artist, 0);
        add_text_line(&lyr_text, title, 0);
        add_text_line(&lyr_text, "", 0);
        add_text_line(&lyr_text, "", 0);
        
        if (((retrieval_spec*)c)->way != -1) /*till it'S of use*/
        {
                 if(get_lyr_by_src (src_selection, artist, title) != 0) return NULL;
        }
        /*else{
                if(get_lyr_hd(artist, title) != 0) 
                {
                if(get_lyr_hd(artist, title) != 0) return NULL;
                }
                else result |= 1;
        }*/
        
        lw->start = 0;
        check_repaint();
        lock = 1;
        return &lyr_text;
}	

static char *
list_callback(int index, int *highlight, void *data)
{
        static char buf[512];
        
    //i think i'ts fine to write it into the 1st line...
  if((index == lyr_text.lines->len && lyr_text.lines->len > 4)||
          ((lyr_text.lines->len == 0 
          ||lyr_text.lines->len == 4) && index == 0))
  {
    *highlight=3; 
    src_lyr* selected =  g_array_index (src_lyr_stack, src_lyr*, src_selection);
    if (selected != NULL) return selected->description;
    return "";
  }
    
  if(index < 2 && lyr_text.lines->len > 4) *highlight=3;
  else if(index >=  lyr_text.lines->len ||
        ( index < 4 && index != 0 && lyr_text.lines->len < 5))
  {
          return "";
  }
 
  get_text_line(&lyr_text, index, buf, 512);
  return buf;
} 


static void
lyrics_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
  lw->flags = LW_HIDE_CURSOR;
  //lyr_text.lines = g_array_new(FALSE, TRUE, 4);
  formed_text_init(&lyr_text);
  if (!g_thread_supported()) g_thread_init(NULL);
  
}

static void
lyrics_resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}

static void
lyrics_exit(void)
{
  list_window_free(lw);
}


static char *
lyrics_title(char *str, size_t size)
{
  static GString *msg;
  if (msg == NULL)
    msg = g_string_new ("");
  else g_string_erase (msg, 0, -1);

  g_string_append (msg, "Lyrics  [");

  if (src_selection > src_lyr_stack->len-1)
    g_string_append (msg, "No plugin available");
  else
    {
      src_lyr* selected =  g_array_index (src_lyr_stack, src_lyr*, src_selection);
      if (selected != NULL)
	  g_string_append (msg, selected->name);
      else g_string_append (msg, "NONE");
    }
  if(lyr_text.lines->len == 4)
	{
                if(lock == 1)
                {
                        if(!(result & 1))
                        {
				g_string_append (msg, " - ");
                                if(!(result & 2)) g_string_append (msg, _("No access"));
				else if(!(result & 4)||!(result & 16)) g_string_append (msg, _("Not found")); 
                        }
                }
                if(lock == 2) 
		  {
		    g_string_append (msg, " - ");
		    g_string_append (msg, _("retrieving"));
		  }
        }

	g_string_append_c (msg, ']');

	return msg->str;
}

static void 
lyrics_paint(screen_t *screen, mpdclient_t *c)
{
  lw->clear = 1;
  list_window_paint(lw, list_callback, NULL);
  wrefresh(lw->w);
}


static void 
lyrics_update(screen_t *screen, mpdclient_t *c)
{  
  if( lw->repaint )
    {
      list_window_paint(lw, list_callback, NULL);
      wrefresh(lw->w);
      lw->repaint = 0;
    }
}


static int 
lyrics_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
  lw->repaint=1;
  static retrieval_spec spec;
  switch(cmd)
    {
    case CMD_LIST_NEXT:
      if( lw->start+lw->rows < lyr_text.lines->len+1 )
        lw->start++;
      return 1;
    case CMD_LIST_PREVIOUS:
      if( lw->start >0 )
        lw->start--;
      return 1;
    case CMD_LIST_FIRST:
      lw->start = 0;
      return 1;
    case CMD_LIST_LAST:
      lw->start = lyrics_text_rows-lw->rows;
      if( lw->start<0 )
        lw->start = 0;
      return 1;
    case CMD_LIST_NEXT_PAGE:
      lw->start = lw->start + lw->rows-1;
      if( lw->start+lw->rows >= lyr_text.lines->len+1 )
        lw->start = lyr_text.lines->len-lw->rows+1;
      if( lw->start<0 )
        lw->start = 0;
       return 1;
    case CMD_LIST_PREVIOUS_PAGE:
      lw->start = lw->start - lw->rows;
      if( lw->start<0 )
        lw->start = 0;
      return 1;
        case CMD_SELECT:
          spec.client = c;
          spec.way = 0;
          g_thread_create(get_lyr, &spec, FALSE, NULL);	
          return 1;
        case CMD_INTERRUPT:
          if(lock > 1) lock = 4;
          return 1;	
        case CMD_ADD:
          if(lock > 0 && lock != 4)
          {
                   if(store_lyr_hd() == 0) screen_status_message (_("Lyrics saved!"));
          }
          return 1;
        case CMD_LYRICS_UPDATE:
          spec.client = c;
          spec.way = 1;
          g_thread_create(get_lyr, &spec, FALSE, NULL);
	  return 1;
	case CMD_SEARCH_MODE:
	  //while (0==0) fprintf (stderr, "%i", src_lyr_stack->len);
	  if (src_selection == src_lyr_stack->len-1) src_selection = -1;
	  src_selection++;
	  return 1;
	default:
	  break;
    }

  lw->selected = lw->start+lw->rows;
  if( screen_find(screen, c, 
                  lw,  lyrics_text_rows,
                  cmd, list_callback, NULL) )
    {
      /* center the row */
      lw->start = lw->selected-(lw->rows/2);
      if( lw->start+lw->rows > lyrics_text_rows )
        lw->start = lyrics_text_rows-lw->rows;
      if( lw->start<0 )
        lw->start=0;
      return 1;
    }

  return 0;
}

static list_window_t *
lyrics_lw(void)
{
  return lw;
}

screen_functions_t *
get_screen_lyrics(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = lyrics_init;
  functions.exit   = lyrics_exit;
  functions.open   = NULL;
  functions.close  = NULL;
  functions.resize = lyrics_resize;
  functions.paint  = lyrics_paint;
  functions.update = lyrics_update;
  functions.cmd    = lyrics_cmd;
  functions.get_lw = lyrics_lw;
  functions.get_title = lyrics_title;

  return &functions;
}
#endif /* ENABLE_LYRICS_SCREEN */
