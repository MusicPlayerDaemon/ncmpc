/* 
 * $Id: screen_lyrics.c 3355 2006-09-1 17:44:04Z tradiaz $
 *	
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
 * Tue Aug  1 23:17:38 2006
 * lyrics enhancement written by Andreas Obergrusberger <tradiaz@yahoo.de> 
 * using www.leoslyrics.com XML API
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


#define LEOSLYRICS_SEARCH_URL "http://api.leoslyrics.com/api_search.php?auth=ncmpc&artist=%s&songtitle=%s"
 
#define LEOSLYRICS_CONTENT_URL "http://api.leoslyrics.com/api_lyrics.php?auth=ncmpc&hid=%s"

#define CREDITS "Lyrics provided by www.LeosLyrics.com"
typedef struct _formed_text
{
	GString *text;
	GArray *lines;
	int val;
} formed_text;

typedef struct _retrieval_spec
{
	mpdclient_t *client;
	int way;
} retrieval_spec;



XML_Parser parser, contentp;
static int lyrics_text_rows = -1;
static list_window_t *lw = NULL;
guint8 result;
char *hid;
GTimer *dltime;
short int lock;
formed_text lyr_text;
/* result is a bitset in which the succes when searching 4 lyrics is logged
countend by position - backwards
0: lyrics in database
1: proper access  to the lyrics provider
2: lyrics found
3: exact match
4: lyrics downloaded
5: lyrics saved
wasting 3 bits doesn't mean being a fat memory hog like kde.... does it?
*/
static void lyrics_paint(screen_t *screen, mpdclient_t *c);

int get_text_line(formed_text *text, int num, char *dest, int len)
{
	memset(dest, '\0', len*sizeof(char));
    if(num >= text->lines->len-1) return -1;	
	int linelen;
	if(num == 0)
	{
		linelen = g_array_index(text->lines, int, num);
		memcpy(dest, text->text->str, linelen*sizeof(char));	
	}
	else if(num == 1)
	{ //dont ask me why, but this is needed....
		linelen = g_array_index(text->lines, int, num)
			- g_array_index(text->lines, int, num-1);
		memcpy(dest, &text->text->str[g_array_index(text->lines, int, num-1)],
		linelen*sizeof(char));	
	}	
	else
	{
		linelen = g_array_index(text->lines, int, num+1)
			- g_array_index(text->lines, int, num);
		memcpy(dest, &text->text->str[g_array_index(text->lines, int, num)],
		linelen*sizeof(char));	
	}
	dest[linelen] = '\n';
	dest[linelen+1] = '\0';
	
	return 0;
}
	
void add_text_line(formed_text *dest, const char *src, int len)
{
	// need this because g_array_append_val doesnt work with literals
	// and expat sends "\n" as an extra line everytime
	if(len == 0)
	{
		dest->val = strlen(src);
		if(dest->lines->len > 0) dest->val += g_array_index(dest->lines, int,
									dest->lines->len-1);
		g_string_append(dest->text, src);
		g_array_append_val(dest->lines, dest->val);
		return;
	}
	if(len > 1 || dest->val == 0) 
	{
		dest->val = len;	
		if(dest->lines->len > 0) dest->val += g_array_index(dest->lines, int,
									dest->lines->len-1);
	}
	else if (len == 1 && dest->val != 0) dest->val = 0;
				
	if(dest->val > 0)
	{ 
		g_string_append_len(dest->text, src, len);
		g_array_append_val(dest->lines, dest->val);
	}
}

void formed_text_init(formed_text *text)
{
	if(text->text != NULL) g_string_free(text->text, TRUE);	
	text->text = g_string_new("");
	
	if(text->lines != NULL) g_array_free(text->lines, TRUE);
	text->lines = g_array_new(FALSE, TRUE, 4);
	
	text->val = 0;
}

static void check_content(void *data, const char *name, const char **atts)
{ 
	if(strstr(name, "text") != NULL)
	{

		result |= 16;
	}
}
	

static void check_search_response(void *data, const char *name,
		 const char **atts)
{
	if(strstr(name, "response") != NULL)
	{
	result |=2;
	return;
	}  
    	
	if(result & 4)
	{
		if(strstr(name, "result") != NULL)
		{
			if(strstr(atts[2], "hid") != NULL)
			{
				hid = atts[3];
			}
	
			if(strstr(atts[2], "exactMatch") != NULL)
			{
				result |= 8;
			}			
		}
	}
			
}

static void end_tag(void *data, const char *name)
{
  //hmmmmmm		
}

  static void check_search_success(void *userData, const XML_Char *s, int len)
    {
	if(result & 2)	//lets first check whether we're right
	{		//we don't really want to search in the wrong string
		if(strstr((char*) s, "SUCCESS"))
		{
		result |=4;
		}
	}	
    }

static void fetch_text(void *userData, const XML_Char *s, int len) 
{
	if(result & 16)
	{
		add_text_line(&lyr_text, s, len); 
	}
}

int check_dl_progress(void *clientp, double dltotal, double dlnow,
                        double ultotal, double ulnow)
{
	if(g_timer_elapsed(dltime, NULL) >= options.lyrics_timeout || lock == 4)
	{	
		formed_text_init(&lyr_text);
		return -1;
	}

	return 0;
}	


int check_lyr_http(char *artist, char *title, char *url)
{
	char url_avail[256];

	//this replacess the whitespaces with '+'
	g_strdelimit(artist, " ", '+');
	g_strdelimit(title, " ", '+');
	
	//we insert the artist and the title into the url		
	snprintf(url_avail, 512, LEOSLYRICS_SEARCH_URL, artist, title);

	//download that xml!
	easy_download_struct lyr_avail = {NULL, 0,-1};	
	
	g_timer_start(dltime);
	if(!easy_download(url_avail, &lyr_avail, check_dl_progress)) return -1;
	g_timer_stop(dltime);

	//we gotta parse that stuff with expat
	parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, NULL);
	
	XML_SetElementHandler(parser, check_search_response, end_tag);
	XML_SetCharacterDataHandler(parser, check_search_success);
	XML_Parse(parser, lyr_avail.data, strlen(lyr_avail.data), 0);	
	XML_ParserFree(parser);	

	if(!(result & 4)) return -1; //check whether lyrics found
	snprintf(url, 512, LEOSLYRICS_CONTENT_URL, hid);

	return 0;
}
int get_lyr_http(char *artist, char *title)
{
	char url_hid[256];
	if(dltime == NULL) dltime = g_timer_new();

	if(check_lyr_http(artist, title, url_hid) != 0) return -1;
	
	easy_download_struct lyr_content = {NULL, 0,-1};  
	g_timer_continue(dltime);		
	if(!(easy_download(url_hid, &lyr_content, check_dl_progress))) return -1;
	g_timer_stop(dltime);
	
	contentp = XML_ParserCreate(NULL);
	XML_SetUserData(contentp, NULL);
	XML_SetElementHandler(contentp, check_content, end_tag);	
	XML_SetCharacterDataHandler(contentp, fetch_text);
	XML_Parse(contentp, lyr_content.data, strlen(lyr_content.data), 0);
	XML_ParserFree(contentp);

	return 0;
	
}
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

char *check_lyr_hd(char *artist, char *title, int how)
{ //checking whether for lyrics file existence and proper access
	static char path[1024];
	snprintf(path, 1024, "%s/.lyrics/%s/%s.lyric", 
			getenv("HOME"), artist, title);
    
    if(g_access(path, how) != 0) return NULL;
		 
	return path;
}		


int get_lyr_hd(char *artist, char *title)
{
	char *path = check_lyr_hd(artist, title, R_OK);
	if(path == NULL) return -1;
	
	FILE *lyr_file;	
	lyr_file = fopen(path, "r");
	if(lyr_file == NULL) return -1;
	
	char *buf = NULL;
	char **line = &buf;
	size_t n = 0;
	
	while(1)
	{
	 n = getline(line, &n, lyr_file); 
	 if( n < 1 || *line == NULL || feof(lyr_file) != 0 ) return 0;
	 add_text_line(&lyr_text, *line, n);
	 free(*line);
	 *line = NULL; n = 0;
 	}
	
	return 0;
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
	
	if (((retrieval_spec*)c)->way == 1)
	{
		 if(get_lyr_http(artist, title) != 0) return NULL;
	}
	else{
		if(get_lyr_hd(artist, title) != 0) 
		{
		if(get_lyr_http(artist, title) != 0) return NULL;
		}
		else result |= 1;
	}
	
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
  	return CREDITS;
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
	if(lyr_text.lines->len == 4)
	{
		if(lock == 1)
		{
			if(!(result & 1))
			{
				if(!(result & 2)) return _("Lyrics  [No connection]");
				if(!(result & 4)) return _("Lyrics  [Not found]"); 
			}
		}
		if(lock == 2) return _("Lyrics  [retrieving]");
	}
	/*if(lyr_text.lines->len > 2) 
	{
		static char buf[512];
		char artist[512];
		char title[512];
		get_text_line(&lyr_text, 0, artist, 512);
		get_text_line(&lyr_text, 1, artist, 512);
		snprintf(buf, 512, "Lyrics  %s - %s", artist, title);
		return buf;
	}*/
	return "Lyrics";
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
