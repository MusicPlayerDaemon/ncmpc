/* 
 * $Id$
 *
 * (c) 2005 by Kalle Wallin <kaw@linux.se>
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

typedef enum { LIST_ARTISTS, LIST_ALBUMS, LIST_SONGS } artist_mode_t;

static artist_mode_t mode = LIST_ARTISTS;
static char *artist = NULL;
static char *album  = NULL;
static list_window_t *lw = NULL;
static mpdclient_filelist_t *filelist = NULL;
static int metalist_length = 0;
static GList *metalist = NULL;
static list_window_state_t *lw_state = NULL;

static gint
compare_utf8(gconstpointer s1, gconstpointer s2)
{
  char *key1, *key2;
  int n;

  key1 = g_utf8_collate_key(s1,-1);
  key2 = g_utf8_collate_key(s2,-1);
  n = strcmp(key1,key2);
  g_free(key1);
  g_free(key2);
  return n;
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

/* fetch artists/albums/songs from mpd */
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
						    TRUE,
						    MPD_TABLE_ARTIST,
						    artist);
	}
      else
	filelist = mpdclient_filelist_search_utf8(c,  
						  TRUE,
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
      mode = LIST_SONGS;
    }
  else if( m_artist ) /* retreive albums... */
    {
      artist = m_artist;
      metalist = mpdclient_get_albums_utf8(c, m_artist);
      /* sort list */
      metalist = g_list_sort(metalist, compare_utf8);
      /* add a dummy entry for ".." */
      metalist = g_list_insert(metalist, g_strdup(".."), 0);
      /* add a dummy entry for all songs */
      metalist = g_list_insert(metalist, g_strdup(_("All tracks")), -1);
      mode = LIST_ALBUMS;
    }
  else /* retreive artists... */
    {
      metalist = mpdclient_get_artists_utf8(c);
      /* sort list */
      metalist = g_list_sort(metalist, compare_utf8);
      mode = LIST_ARTISTS;
    }
  metalist_length = g_list_length(metalist);
  lw->clear = TRUE;
}

/* db updated */
static void 
browse_callback(mpdclient_t *c, int event, gpointer data)
{
  switch(event)
    {
    case BROWSE_DB_UPDATED:
      D("screen_artist.c> browse_callback() [BROWSE_DB_UPDATED]\n");
      lw->clear = 1;
      lw->repaint = 1;
      update_metalist(c, g_strdup(artist), g_strdup(album));
      break;
    default:
      break;
    }
}

static void
init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
  lw_state = list_window_init_state();
  artist = NULL;
  album = NULL;
}

static void
quit(void)
{
  if( filelist )
    filelist = mpdclient_filelist_free(filelist);
  if( metalist )
    metalist = string_list_free(metalist);
  g_free(artist);
  g_free(album);
  artist = NULL;
  album = NULL;
  lw = list_window_free(lw);  
  lw_state = list_window_free_state(lw_state);
}

static void
open(screen_t *screen, mpdclient_t *c)
{
  static gboolean callback_installed = FALSE;

  if( metalist==NULL && filelist ==NULL)
    update_metalist(c, NULL, NULL);
  if( !callback_installed )
    {
      mpdclient_install_browse_callback(c, browse_callback);
      callback_installed = TRUE;
    }
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
  if( filelist && !filelist->updated )
    {
      list_window_paint(lw, browse_lw_callback, (void *) filelist);
    }
  else if( metalist )
    {
      list_window_paint(lw, artist_lw_callback, (void *) metalist);
    }
  else
    {
      paint(screen, c);
    }
  wnoutrefresh(lw->w);
}

static char *
get_title(char *str, size_t size)
{
  char *s1 = artist ? utf8_to_locale(artist) : NULL;
  char *s2 = album ? utf8_to_locale(album) : NULL;

  switch(mode)
    {
    case LIST_ARTISTS:
      g_snprintf(str, size,  _("Artist: [db browser - EXPERIMENTAL]"));
      break;
    case LIST_ALBUMS:
      g_snprintf(str, size,  _("Artist: %s"), s1);
      break;
    case LIST_SONGS:
      g_snprintf(str, size,  _("Artist: %s - %s"), s1, s2);
      break;
    }
  g_free(s1);
  g_free(s2);
  return str;
}

static list_window_t *
get_filelist_window()
{
  return lw;
}

static void
add_query(mpdclient_t *c, int table, char *filter)
{
  char *str;
  mpdclient_filelist_t *addlist;

  str = utf8_to_locale(filter);
  if( table== MPD_TABLE_ALBUM )
    screen_status_printf("Adding album %s...", str);
  else
    screen_status_printf("Adding %s...", str);
  g_free(str);
  addlist = mpdclient_filelist_search_utf8(c, TRUE, table, filter);
  if( addlist )
    {
      mpdclient_filelist_add_all(c, addlist);
      addlist = mpdclient_filelist_free(addlist);
    }
}

static int 
artist_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
  char *selected;

  switch(cmd)
    {
    case CMD_PLAY:
      switch(mode)
	{
	case LIST_ARTISTS:
	  selected = (char *) g_list_nth_data(metalist, lw->selected);
	  update_metalist(c, g_strdup(selected), NULL);
	  list_window_push_state(lw_state,lw); 
	  break;
	case LIST_ALBUMS:
	  if( lw->selected == 0 )  /* handle ".." */
	    {
	      update_metalist(c, NULL, NULL);
	      list_window_reset(lw);
	      /* restore previous list window state */
	      list_window_pop_state(lw_state,lw); 
	    }
	  else if( lw->selected == metalist_length-1) /* handle "show all" */
	    {
	      update_metalist(c, g_strdup(artist), g_strdup("\0"));
	      list_window_push_state(lw_state,lw); 
	    }
	  else /* select album */
	    {
	      selected = (char *) g_list_nth_data(metalist, lw->selected);
	      update_metalist(c, g_strdup(artist), g_strdup(selected));
	      list_window_push_state(lw_state,lw); 
	    }
	  break;
	case LIST_SONGS:
	  if( lw->selected==0 )  /* handle ".." */
	    {
	      update_metalist(c, g_strdup(artist), NULL);
	      list_window_reset(lw);
	      /* restore previous list window state */
	      list_window_pop_state(lw_state,lw); 
	    }
	  else
	    browse_handle_enter(screen, c, lw, filelist);
	  break;
	}
      return 1;


    /* FIXME? CMD_GO_* handling duplicates code from CMD_PLAY */

    case CMD_GO_PARENT_DIRECTORY:
      switch(mode)
	{
	case LIST_ALBUMS:
	  update_metalist(c, NULL, NULL);
	  list_window_reset(lw);
	  /* restore previous list window state */
	  list_window_pop_state(lw_state,lw);
	  break;
	case LIST_SONGS:
	  update_metalist(c, g_strdup(artist), NULL);
	  list_window_reset(lw);
	  /* restore previous list window state */
	  list_window_pop_state(lw_state,lw);
	  break;
	}
      break;

    case CMD_GO_ROOT_DIRECTORY:
      switch(mode)
	{
	case LIST_ALBUMS:
	case LIST_SONGS:
	  update_metalist(c, NULL, NULL);
	  list_window_reset(lw);
	  /* restore first list window state (pop while returning true) */
	  while(list_window_pop_state(lw_state,lw));
	  break;
	}
      break;

    case CMD_SELECT:
      switch(mode)
	{
	case LIST_ARTISTS:
	  selected = (char *) g_list_nth_data(metalist, lw->selected);
	  add_query(c, MPD_TABLE_ARTIST, selected);
	  cmd = CMD_LIST_NEXT; /* continue and select next item... */
	  break;
	case LIST_ALBUMS:
	  if( lw->selected && lw->selected == metalist_length-1)
	    {
	      add_query(c, MPD_TABLE_ARTIST, artist);
	    }
	  else if( lw->selected > 0 )
	    {
	      selected = (char *) g_list_nth_data(metalist, lw->selected);
	      add_query(c, MPD_TABLE_ALBUM, selected);
	      cmd = CMD_LIST_NEXT; /* continue and select next item... */
	    }
	  break;
	case LIST_SONGS:
	  if( browse_handle_select(screen, c, lw, filelist) == 0 )
	    {
	      cmd = CMD_LIST_NEXT; /* continue and select next item... */
	    }
	  break;
	}
      break;

      /* continue and update... */
    case CMD_SCREEN_UPDATE:
      screen->painted = 0;
      lw->clear = 1;
      lw->repaint = 1;
      update_metalist(c, g_strdup(artist), g_strdup(album));
      screen_status_printf(_("Screen updated!"));
      return 0;

    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      if( filelist )
	return screen_find(screen,
			   lw, filelist->length,
			   cmd, browse_lw_callback, (void *) filelist);
      else if ( metalist )
	return screen_find(screen,
			   lw, metalist_length,
			   cmd, artist_lw_callback, (void *) metalist);
      else
	return 1;

    case CMD_MOUSE_EVENT:
      return browse_handle_mouse_event(screen,c,lw,filelist);

    default:
      break;
    }

  if( filelist )
    return list_window_cmd(lw, filelist->length, cmd);
  else if( metalist )
    return list_window_cmd(lw, metalist_length, cmd);

  
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
