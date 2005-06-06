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
#ifndef DISABLE_ARTIST_SCREEN
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "mpdclient.h"
#include "utils.h"
#include "strfsong.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_browse.h"

#define BUFSIZE 1024

static char *artist = NULL;
static char *album  = NULL;
static list_window_t *lw = NULL;
static mpdclient_filelist_t *filelist = NULL;
static int metalist_length = 0;
static GList *metalist = NULL;
static GList *lw_state_list = NULL;

/* store current state when entering a subdirectory */
static void
push_lw_state(void)
{
  list_window_t *tmp = g_malloc(sizeof(list_window_t));

  memcpy(tmp, lw, sizeof(list_window_t));
  lw_state_list = g_list_prepend(lw_state_list, (gpointer) tmp);
}

/* get previous state when leaving a directory */
static void
pop_lw_state(void)
{
  if( lw_state_list )
    {
      list_window_t *tmp = lw_state_list->data;

      memcpy(lw, tmp, sizeof(list_window_t));
      g_free(tmp);
      lw_state_list->data = NULL;
      lw_state_list = g_list_delete_link(lw_state_list, lw_state_list);
    }
}

/* list_window callback */
static char *
artist_lw_callback(int index, int *highlight, void *data)
{
  static char buf[BUFSIZE];
  char *str, *str_utf8;
  
  *highlight = 0;
  if( (str_utf8=(char *) g_list_nth_data(metalist,index))==NULL )
    return NULL;

  str = utf8_to_locale(str_utf8);
  g_snprintf(buf, BUFSIZE, "[%s]", str);
  g_free(str);

  return buf;
}

/* the playlist have been updated -> fix highlights */
static void 
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
  if( filelist==NULL )
    return;
  D("screen_artist.c> playlist_callback() [%d]\n", event);
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

static void
update_metalist(mpdclient_t *c, char *m_artist, char *m_album)
{
  g_free(artist);
  g_free(album);
  artist = NULL;
  album = NULL;
  if( metalist )
    metalist = string_list_free(metalist);
  if (filelist ) {
    mpdclient_remove_playlist_callback(c, playlist_changed_callback);
    filelist = mpdclient_filelist_free(filelist);
  }
  if( m_album ) /* retreive songs... */
    {
      artist = m_artist;
      album = m_album;
      if( album[0] == 0 )
	{
	  album = g_strdup(_("All tracks"));
	  filelist = mpdclient_filelist_search_utf8(c,  
						    MPD_TABLE_ARTIST,
						    artist);
	}
      else
	filelist = mpdclient_filelist_search_utf8(c,  
						  MPD_TABLE_ALBUM,
						  album);
      /* add a dummy entry for ".." */
      filelist_entry_t *entry = g_malloc0(sizeof(filelist_entry_t));
      entry->entity = NULL;
      filelist->list = g_list_insert(filelist->list, entry, 0);
      filelist->length++;
      /* install playlist callback and fix highlights */
      sync_highlights(c, filelist);
      mpdclient_install_playlist_callback(c, playlist_changed_callback);
    }
  else if( m_artist ) /* retreive albums... */
    {
      artist = m_artist;
      metalist = mpdclient_get_albums_utf8(c, m_artist);
      /* add a dummy entry for ".." */
      metalist = g_list_insert(metalist, g_strdup(".."), 0);
      /* add a dummy entry for all songs */
      metalist = g_list_insert(metalist, g_strdup(_("All tracks")), -1);

    }
  else /* retreive artists... */
    {
      metalist = mpdclient_get_artists_utf8(c);
    }
  metalist_length = g_list_length(metalist);
}

static void
init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
  artist = NULL;
  album = NULL;
}

static void
quit(void)
{
  /* clear list window state */
  if( lw_state_list )
    {
      GList *list = lw_state_list;
      while( list )
	{
	  g_free(list->data);
	  list->data = NULL;
	  list = list->next;
	}
      g_list_free(lw_state_list);
      lw_state_list = NULL;
    }
  
  if( filelist )
    filelist = mpdclient_filelist_free(filelist);
  if( metalist )
    metalist = string_list_free(metalist);
  g_free(artist);
  g_free(album);
  artist = NULL;
  album = NULL;
  list_window_free(lw);  
}

static void
open(screen_t *screen, mpdclient_t *c)
{
  if( metalist==NULL && filelist ==NULL)
    update_metalist(c, NULL, NULL);
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
      list_window_paint(lw, browse_lw_callback, (void *) filelist);
      filelist->updated = FALSE;
    }
  else if( metalist )
    {
      list_window_paint(lw, artist_lw_callback, (void *) metalist);
    }
  else
    {
      wmove(lw->w, 0, 0);
      wclrtobot(lw->w);
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
  char *s1 = artist ? utf8_to_locale(artist) : NULL;
  char *s2 = album ? utf8_to_locale(album) : NULL;

  if( album )
    g_snprintf(str, size,  _("Artist: %s - %s"), s1, s2);
  else if( artist )
    g_snprintf(str, size,  _("Artist: %s"), s1);
  else
    g_snprintf(str, size,  _("Artist: [db browser - EXPERIMENTAL]"));
  g_free(s1);
  g_free(s2);
  return str;
}

static list_window_t *
get_filelist_window()
{
  return lw;
}

static int 
artist_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
  switch(cmd)
    {
    case CMD_PLAY:
      if( artist && album )
	{
	  if( lw->selected==0 )  /* handle ".." */
	    {
	      update_metalist(c, g_strdup(artist), NULL);
	      list_window_reset(lw);
	      /* restore previous list window state */
	      pop_lw_state(); 
	    }
	  else
	    browse_handle_enter(screen, c, lw, filelist);
	}
      else if ( artist )
	{
	  if( lw->selected == 0 )  /* handle ".." */

	    {
	      update_metalist(c, NULL, NULL);
	      list_window_reset(lw);
	      /* restore previous list window state */
	      pop_lw_state(); 
	    }
	  else if( lw->selected == metalist_length-1) /* handle "show all" */
	    {
	      update_metalist(c, g_strdup(artist), g_strdup("\0"));
	      push_lw_state(); 
	      list_window_reset(lw);
	    }
	  else /* select album */
	    {
	      char *selected = (char *) g_list_nth_data(metalist, lw->selected);
	      update_metalist(c, g_strdup(artist), g_strdup(selected));
	      push_lw_state(); 
	      list_window_reset(lw);
	    }
	}
      else
	{
	  char *selected = (char *) g_list_nth_data(metalist, lw->selected);
	  update_metalist(c, g_strdup(selected), NULL);
	  push_lw_state(); 
	  list_window_reset(lw);
	}
      return 1;

    case CMD_SELECT:
      if( browse_handle_select(screen, c, lw, filelist) == 0 )
	{
	  /* continue and select next item... */
	  cmd = CMD_LIST_NEXT;
	}
      return 1;

      /* continue and update... */
    case CMD_SCREEN_UPDATE:
      return 0;

    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      if( filelist )
	return screen_find(screen, c, 
			   lw, filelist->length,
			   cmd, browse_lw_callback, (void *) filelist);
      else if ( metalist )
	return screen_find(screen, c, 
			   lw, metalist_length,
			   cmd, artist_lw_callback, (void *) metalist);
      else
	return 1;

    case CMD_MOUSE_EVENT:
      return browse_handle_mouse_event(screen,c,lw,filelist);

    default:
      if( filelist )
	return list_window_cmd(lw, filelist->length, cmd);
      else if( metalist )
	return list_window_cmd(lw, metalist_length, cmd);
    }
  
  return 0;
}

screen_functions_t *
get_screen_artist(void)
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
  functions.cmd    = artist_cmd;
  functions.get_lw = get_filelist_window;
  functions.get_title = get_title;

  return &functions;
}


#endif /* ENABLE_ARTIST_SCREEN */
