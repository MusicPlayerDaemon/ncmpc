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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#ifndef DISABLE_SEARCH_SCREEN
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "mpdclient.h"
#include "strfsong.h"
#include "command.h"
#include "screen.h"
#include "utils.h"
#include "screen_utils.h"
#include "screen_browse.h"

/* new search stuff with qball's libmpdclient */
#define FUTURE


#ifdef FUTURE

typedef struct
{
  int id;
  char *name;
  char *localname;
} search_tag_t;

static search_tag_t search_tag[] = {
  { MPD_TAG_ITEM_ARTIST,   "artist",    N_("artist") },
  { MPD_TAG_ITEM_ALBUM,    "album",     N_("album") },
  { MPD_TAG_ITEM_TITLE,    "title",     N_("title") },
  { MPD_TAG_ITEM_TRACK,    "track",     N_("track") },
  { MPD_TAG_ITEM_NAME,     "name",      N_("name") },
  { MPD_TAG_ITEM_GENRE,    "genre",     N_("genre") },
  { MPD_TAG_ITEM_DATE,     "date",      N_("date") },
  { MPD_TAG_ITEM_COMPOSER, "composer",  N_("composer") },
  { MPD_TAG_ITEM_PERFORMER,"performer", N_("performer") },
  { MPD_TAG_ITEM_COMMENT,  "comment",   N_("comment") },
  { MPD_TAG_ITEM_FILENAME, "filename",  N_("file") },
  { -1,                    NULL,        NULL }
};

static int
search_get_tag_id(char *name)
{
  int i;

  i=0;
  while( search_tag[i].name )
    {
      if( strcasecmp(search_tag[i].name, name)==0 || 
	  strcasecmp(search_tag[i].localname, name)==0 )
	return search_tag[i].id;
      i++;
    }
  return -1;
}

#endif


#define SEARCH_TITLE    0
#define SEARCH_ARTIST   1
#define SEARCH_ALBUM    2
#define SEARCH_FILE     3

typedef struct {
  int table;
  char *label;
} search_type_t;

static search_type_t mode[] = {
  { MPD_TABLE_TITLE,    N_("Title") },
  { MPD_TABLE_ARTIST,   N_("Artist") },
  { MPD_TABLE_ALBUM,    N_("Album") },
  { MPD_TABLE_FILENAME, N_("Filename") },
  { 0, NULL }
};

static list_window_t *lw = NULL;
static mpdclient_filelist_t *filelist = NULL;
static GList *search_history = NULL;
static gchar *pattern = NULL;
static gboolean advanced_search_mode = FALSE;


/* search info */
static char *
lw_search_help_callback(int index, int *highlight, void *data)
{
  int text_rows;
  static char *text[] = {
    "Welcome to ncmpc's search screen - SVN version.",
    "",
    "Quick search - just enter a string and ncmcp will search according",
    "               to the current search mode (displayed above).",
    "",
    "Advanced - bla bla bla.... syntax below",
    ""
    " <tag>:<search term> [<tag>:<search term>...]",
    "",
    "Example: artist:radiohead album:pablo honey",
    "",
    "##### SOMEONE - Write a proper help text, please! #####",
    "",
    "avalible tags: artist, album, title, track, name, genre, date",
    "               composer, performer, comment, file",
    "",
    NULL
  };

  text_rows=0;
  while( text[text_rows] )
    text_rows++;
  
  if( index < text_rows )
    return text[index];
  return NULL;
}

/* the playlist have been updated -> fix highlights */
static void 
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
  if( filelist==NULL )
    return;
  D("screen_search.c> playlist_callback() [%d]\n", event);
  switch(event)
    {
    case PLAYLIST_EVENT_CLEAR:
      clear_highlights(filelist);
      break;
    default:
      sync_highlights(c, filelist);
      break;
    }
}

/* sanity check search mode value */
static void
search_check_mode(void)
{
  int max = 0;

  while( mode[max].label != NULL )
    max++;
  if( options.search_mode<0 )
    options.search_mode = 0;
  else if( options.search_mode>=max )
    options.search_mode = max-1;
}

static void
search_clear(screen_t *screen, mpdclient_t *c, gboolean clear_pattern)
{
  if( filelist )
    {
      mpdclient_remove_playlist_callback(c, playlist_changed_callback);
      filelist = mpdclient_filelist_free(filelist);
    }
  if( clear_pattern && pattern )
    {
      g_free(pattern);
      pattern = NULL;
    }
}

#ifdef FUTURE
/*-----------------------------------------------------------------------
 * NOTE: This code exists to test a new search ui,
 *       Its ugly and MUST be redesigned before the next release!
 *-----------------------------------------------------------------------
 */
static mpdclient_filelist_t *
search_advanced_query(char *query, mpdclient_t *c)
{
  int i,j;
  char **strv;
  int table[10];
  char *arg[10];
  mpdclient_filelist_t *filelist = NULL;

  advanced_search_mode = FALSE;
  if( g_strrstr(query, ":") == NULL )
    return NULL;
  
  strv = g_strsplit_set(query, ": ", 0);

  i=0;
  while( strv[i] )
    {
      D("strv[%d] = \"%s\"\n", i, strv[i]);
      i++;
    }

  memset(table, 0, 10*sizeof(int));
  memset(arg,   0, 10*sizeof(char *));

  i=0;
  j=0;
  while( strv[i] && strlen(strv[i])>0 && i<9 )
    {
      D("strv[%d] = \"%s\"\n", i, strv[i]);

      int id = search_get_tag_id(strv[i]);
      if( id==-1 )
	{
	  if( table[j] )
	    {
	      char *tmp = arg[j];
	      arg[j] = g_strdup_printf("%s %s", arg[j], strv[i]);
	      g_free(tmp);
	    }
	  else
	    {
	      D("Bad search tag %s\n", strv[i]);
	      screen_status_printf(_("Bad search tag %s"), strv[i]);
	    }
	  i++;
	}
      else if( strv[i+1] == NULL || strlen(strv[i+1])==0 )
	{
	  D("No argument for search tag %s\n", strv[i]);
	  screen_status_printf(_("No argument for search tag %s"), strv[i]);
	  i++;
	  //	  j--;
	  //table[j] = -1;
	}
      else
	{
	  table[j] = id;
	  arg[j] = locale_to_utf8(strv[i+1]); // FREE ME
	  j++;
	  table[j] = -1;
	  arg[j] = NULL;
	  i = i + 2;
	  advanced_search_mode = TRUE;
	}     
    }

  g_strfreev(strv);


  if( advanced_search_mode && j>0 )
    {
      /*-----------------------------------------------------------------------
       * NOTE (again): This code exists to test a new search ui,
       *               Its ugly and MUST be redesigned before the next release!
       *             + the code below should live in mpdclient.c
       *-----------------------------------------------------------------------
       */
      mpd_InfoEntity *entity;

      /** stupid - but this is just a test...... (fulhack)  */
      mpd_sendSearchTagCommand(c->connection, 
			       table[0], arg[0],
			       table[1], arg[1],
			       table[2], arg[2],
			       table[3], arg[3],
			       table[4], arg[4],
			       table[5], arg[5],
			       table[6], arg[6],
			       table[7], arg[7],
			       table[8], arg[8],
			       table[9], arg[9]);

      filelist = g_malloc0(sizeof(mpdclient_filelist_t));

      while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
	{
	  filelist_entry_t *entry = g_malloc0(sizeof(filelist_entry_t));
      
	  entry->entity = entity;
	  filelist->list = g_list_append(filelist->list, (gpointer) entry);
	  filelist->length++;
	}
  
      if( mpdclient_finish_command(c) && filelist )
	filelist = mpdclient_filelist_free(filelist);

      filelist->updated = TRUE;
    } 
  
  i=0;
  while( arg[i] )
    g_free(arg[i++]);

  return filelist;
}
#else
#define search_advanced_query(pattern,c) (NULL)
#endif

static void
search_new(screen_t *screen, mpdclient_t *c)
{
  search_clear(screen, c, TRUE);
  
  pattern = screen_readln(screen->status_window.w, 
			  _("Search: "),
			  NULL,
			  &search_history,
			  NULL);

  if( pattern && strcmp(pattern,"")==0 )
    {
      g_free(pattern);
      pattern=NULL;
    }
  
  if( pattern==NULL )
    {
      list_window_reset(lw);
      return;
    }

  if( !MPD_VERSION_LT(c, 0, 12, 0) )
    filelist = search_advanced_query(pattern, c);
  if( !advanced_search_mode && filelist==NULL )
    filelist = mpdclient_filelist_search(c, 
					 FALSE,
					 mode[options.search_mode].table,
					 pattern);
  sync_highlights(c, filelist);
  mpdclient_install_playlist_callback(c, playlist_changed_callback);
  list_window_check_selected(lw, filelist->length);
}



static void
init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
}

static void
quit(void)
{
  if( search_history )
    string_list_free(search_history);
  if( filelist )
    filelist = mpdclient_filelist_free(filelist);
  list_window_free(lw);
  if( pattern )
    g_free(pattern);
  pattern = NULL;
}

static void
open(screen_t *screen, mpdclient_t *c)
{
  //  if( pattern==NULL )
  //    search_new(screen, c);
  // else
  screen_status_printf(_("Press %s for a new search"),
			 get_key_names(CMD_SCREEN_SEARCH,0));
  search_check_mode();
}

static void
resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}

static void
close(void)
{
}

static void 
paint(screen_t *screen, mpdclient_t *c)
{
  lw->clear = 1;
  
  if( filelist )
    {
      lw->flags = 0;
      list_window_paint(lw, browse_lw_callback, (void *) filelist);
      filelist->updated = FALSE;
    }
  else
    {
      lw->flags = LW_HIDE_CURSOR;
      list_window_paint(lw, lw_search_help_callback, NULL);
      if( !MPD_VERSION_LT(c, 0, 12, 0) )
	g_strdup_printf("Advanced search disabled (MPD version < 0.12.0"); 
      //      wmove(lw->w, 0, 0);
      //wclrtobot(lw->w);
    }
  wnoutrefresh(lw->w);
}

static void 
update(screen_t *screen, mpdclient_t *c)
{
  if( filelist==NULL || filelist->updated )
    {
      paint(screen, c);
      return;
    }
  list_window_paint(lw, browse_lw_callback, (void *) filelist);
  wnoutrefresh(lw->w);
}

static char *
get_title(char *str, size_t size)
{
  if( advanced_search_mode && pattern )
    g_snprintf(str, size, _("Search: %s"), pattern);
  else if( pattern )
    g_snprintf(str, size, 
	       _("Search: Results for %s [%s]"), 
	       pattern,
	       _(mode[options.search_mode].label));
  else
    g_snprintf(str, size, _("Search: Press %s for a new search [%s]"),
	       get_key_names(CMD_SCREEN_SEARCH,0),
	       _(mode[options.search_mode].label));
	       
  return str;
}

static list_window_t *
get_filelist_window()
{
  return lw;
}

static int 
search_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
  switch(cmd)
    {
    case CMD_PLAY:
       browse_handle_enter(screen, c, lw, filelist);
      return 1;

    case CMD_SELECT:
      if( browse_handle_select(screen, c, lw, filelist) == 0 )
	{
	  /* continue and select next item... */
	  cmd = CMD_LIST_NEXT;
	}
      return 1;

    case CMD_SEARCH_MODE:
      options.search_mode++;
      if( mode[options.search_mode].label == NULL )
	options.search_mode = 0;
      screen_status_printf(_("Search mode: %s"), 
			   _(mode[options.search_mode].label));
      /* continue and update... */
    case CMD_SCREEN_UPDATE:
      if( pattern )
	{
	  search_clear(screen, c, FALSE);
	  filelist = mpdclient_filelist_search(c, 
					       FALSE,
					       mode[options.search_mode].table,
					       pattern);
	  sync_highlights(c, filelist);
	}
      return 1;

    case CMD_SCREEN_SEARCH:
      search_new(screen, c);
      return 1;

    case CMD_CLEAR:
      search_clear(screen, c, TRUE);
      list_window_reset(lw);
      return 1;

    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      if( filelist )
	return screen_find(screen, c, 
			   lw, filelist->length,
			   cmd, browse_lw_callback, (void *) filelist);
      else
	return 1;

    case CMD_MOUSE_EVENT:
      return browse_handle_mouse_event(screen,c,lw,filelist);

    default:
      if( filelist )
	return list_window_cmd(lw, filelist->length, cmd);
    }
  
  return 0;
}

screen_functions_t *
get_screen_search(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = init;
  functions.exit   = quit;
  functions.open   = open;
  functions.close  = close;
  functions.resize = resize;
  functions.paint  = paint;
  functions.update = update;
  functions.cmd    = search_cmd;
  functions.get_lw = get_filelist_window;
  functions.get_title = get_title;

  return &functions;
}


#endif /* ENABLE_SEARCH_SCREEN */
