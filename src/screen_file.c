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
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "mpdclient.h"
#include "strfsong.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"


#define USE_OLD_LAYOUT
#undef  USE_OLD_ADD

#define BUFSIZE 1024

#define HIGHLIGHT  (0x01)


static list_window_t *lw = NULL;
static GList *lw_state_list = NULL;
static mpdclient_filelist_t *filelist = NULL;



/* clear the highlight flag for all items in the filelist */
static void
clear_highlights(mpdclient_filelist_t *filelist)
{
  GList *list = g_list_first(filelist->list);
  
  while( list )
    {
      filelist_entry_t *entry = list->data;

      entry->flags &= ~HIGHLIGHT;
      list = list->next;
    }
}

/* change the highlight flag for a song */
static void
set_highlight(mpdclient_filelist_t *filelist, mpd_Song *song, int highlight)
{
  GList *list = g_list_first(filelist->list);

  if( !song )
    return;

  while( list )
    {
      filelist_entry_t *entry = list->data;
      mpd_InfoEntity *entity  = entry->entity;

      if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song2 = entity->info.song;

	  if( strcmp(song->file, song2->file) == 0 )
	    {
	      if(highlight)
		entry->flags |= HIGHLIGHT;
	      else
		entry->flags &= ~HIGHLIGHT;
	    }
	}
      list = list->next;
    }
}

/* sync highlight flags with playlist */
static void
sync_highlights(mpdclient_t *c, mpdclient_filelist_t *filelist)
{
  GList *list = g_list_first(filelist->list);

  while(list)
    {
      filelist_entry_t *entry = list->data;
      mpd_InfoEntity *entity = entry->entity;

      if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entity->info.song;
	  
	  if( playlist_get_index_from_file(c, song->file) >= 0 )
	    entry->flags |= HIGHLIGHT;
	  else
	    entry->flags &= ~HIGHLIGHT;
	}
      list=list->next;
    }
}

/* the db have changed -> update the filelist */
static void 
file_changed_callback(mpdclient_t *c, int event, gpointer data)
{
  D("screen_file.c> filelist_callback() [%d]\n", event);
  filelist = mpdclient_filelist_update(c, filelist);
  sync_highlights(c, filelist);
  list_window_check_selected(lw, filelist->length);
}

/* the playlist have been updated -> fix highlights */
static void 
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
  D("screen_file.c> playlist_callback() [%d]\n", event);
  switch(event)
    {
    case PLAYLIST_EVENT_CLEAR:
      clear_highlights(filelist);
      break;
    case PLAYLIST_EVENT_ADD:
      set_highlight(filelist, (mpd_Song *) data, 1); 
      break;
    case PLAYLIST_EVENT_DELETE:
      set_highlight(filelist, (mpd_Song *) data, 0); 
      break;
    case PLAYLIST_EVENT_MOVE:
      break;
    default:
      sync_highlights(c, filelist);
      break;
    }
}

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
list_callback(int index, int *highlight, void *data)
{
  static char buf[BUFSIZE];
  //mpdclient_t *c = (mpdclient_t *) data;
  filelist_entry_t *entry;
  mpd_InfoEntity *entity;

  *highlight = 0;
  if( (entry=(filelist_entry_t *)g_list_nth_data(filelist->list,index))==NULL )
    return NULL;

  entity = entry->entity;
  *highlight = (entry->flags & HIGHLIGHT);

  if( entity == NULL )
    {
      return "[..]";
    }
  if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) 
    {
      mpd_Directory *dir = entity->info.directory;
      char *dirname = utf8_to_locale(basename(dir->path));

      snprintf(buf, BUFSIZE, "[%s]", dirname);
      g_free(dirname);
      return buf;
    }
  else if( entity->type==MPD_INFO_ENTITY_TYPE_SONG )
    {
      mpd_Song *song = entity->info.song;

      strfsong(buf, BUFSIZE, LIST_FORMAT, song);
      return buf;
    }
  else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    {
      mpd_PlaylistFile *plf = entity->info.playlistFile;
      char *filename = utf8_to_locale(basename(plf->path));

#ifdef USE_OLD_LAYOUT      
      snprintf(buf, BUFSIZE, "*%s*", filename);
#else 
      snprintf(buf, BUFSIZE, "<Playlist> %s", filename);
#endif
      g_free(filename);
      return buf;
    }
  return "Error: Unknow entry!";
}

/* chdir */
static int
change_directory(screen_t *screen, mpdclient_t *c, filelist_entry_t *entry)
{
  mpd_InfoEntity *entity = entry->entity;
  gchar *path = NULL;

  if( entity==NULL )
    {
      /* return to parent */
      char *parent = g_path_get_dirname(filelist->path);
      if( strcmp(parent, ".") == 0 )
	{
	  parent[0] = '\0';
	}
      path = g_strdup(parent);
      list_window_reset(lw);
      /* restore previous list window state */
      pop_lw_state(); 
    }
  else
    if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY)
      {
	/* enter sub */
	mpd_Directory *dir = entity->info.directory;
	path = utf8_to_locale(dir->path);      
	/* save current list window state */
	push_lw_state(); 
	list_window_reset(lw);
      }
    else
      return -1;

  filelist = mpdclient_filelist_free(filelist);
  filelist = mpdclient_filelist_get(c, path);
  sync_highlights(c, filelist);
  list_window_check_selected(lw, filelist->length);
  g_free(path);
  return 0;
}

static int
load_playlist(screen_t *screen, mpdclient_t *c, filelist_entry_t *entry)
{
  mpd_InfoEntity *entity = entry->entity;
  mpd_PlaylistFile *plf = entity->info.playlistFile;
  char *filename = utf8_to_locale(plf->path);

  if( mpdclient_cmd_load_playlist(c, plf->path) == 0 )
    screen_status_printf(_("Loading playlist %s..."), basename(filename));
  g_free(filename);
  return 0;
}

static int 
handle_delete(screen_t *screen, mpdclient_t *c)
{
  filelist_entry_t *entry;
  mpd_InfoEntity *entity;
  mpd_PlaylistFile *plf;
  char *str, buf[BUFSIZE];
  int key;

  entry=( filelist_entry_t *) g_list_nth_data(filelist->list,lw->selected);
  if( entry==NULL || entry->entity==NULL )
    return -1;

  entity = entry->entity;

  if( entity->type!=MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    {
      screen_status_printf(_("You can only delete playlists!"));
      beep();
      return -1;
    }

  plf = entity->info.playlistFile;
  str = utf8_to_locale(basename(plf->path));
  snprintf(buf, BUFSIZE, _("Delete playlist %s [%s/%s] ? "), str, YES, NO);
  g_free(str);  
  key = tolower(screen_getch(screen->status_window.w, buf));
  if( key==KEY_RESIZE )
    screen_resize();
  if( key != YES[0] )
    {
      screen_status_printf(_("Aborted!"));
      return 0;
    }

  if( mpdclient_cmd_delete_playlist(c, plf->path) )
    {
      return -1;
    }
  screen_status_printf(_("Playlist deleted!"));
  return 0;
}


static int
handle_enter(screen_t *screen, mpdclient_t *c)
{
  filelist_entry_t *entry;
  mpd_InfoEntity *entity;
  
  entry = ( filelist_entry_t *) g_list_nth_data(filelist->list, lw->selected);
  if( entry==NULL )
    return -1;

  entity = entry->entity;
  if( entity==NULL || entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY )
    return change_directory(screen, c, entry);
  else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    return load_playlist(screen, c, entry);
  return -1;
}


#ifdef USE_OLD_ADD
/* NOTE - The add_directory functions should move to mpdclient.c */
extern gint mpdclient_finish_command(mpdclient_t *c);

static int
add_directory(mpdclient_t *c, char *dir)
{
  mpd_InfoEntity *entity;
  GList *subdir_list = NULL;
  GList *list = NULL;
  char *dirname;

  dirname = utf8_to_locale(dir);
  screen_status_printf(_("Adding directory %s...\n"), dirname);
  doupdate(); 
  g_free(dirname);
  dirname = NULL;

  mpd_sendLsInfoCommand(c->connection, dir);
  mpd_sendCommandListBegin(c->connection);
  while( (entity=mpd_getNextInfoEntity(c->connection)) )
    {
      if( entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entity->info.song;
	  mpd_sendAddCommand(c->connection, song->file);
	  mpd_freeInfoEntity(entity);
	}
      else if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY )
	{
	  subdir_list = g_list_append(subdir_list, (gpointer) entity); 
	}
      else
	mpd_freeInfoEntity(entity);
    }
  mpd_sendCommandListEnd(c->connection);
  mpdclient_finish_command(c);
  c->need_update = TRUE;
  
  list = g_list_first(subdir_list);
  while( list!=NULL )
    {
      mpd_Directory *dir;

      entity = list->data;
      dir = entity->info.directory;
      add_directory(c, dir->path);
      mpd_freeInfoEntity(entity);
      list->data=NULL;
      list=list->next;
    }
  g_list_free(subdir_list);
  return 0;
}
#endif

static int
handle_select(screen_t *screen, mpdclient_t *c)
{
  filelist_entry_t *entry;

  entry=( filelist_entry_t *) g_list_nth_data(filelist->list, lw->selected);
  if( entry==NULL || entry->entity==NULL)
    return -1;

  if( entry->entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    return load_playlist(screen, c, entry);

  if( entry->entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY )
    {
      mpd_Directory *dir = entry->entity->info.directory;
#ifdef USE_OLD_ADD
      add_directory(c, dir->path);
#else
      if( mpdclient_cmd_add_path(c, dir->path) == 0 )
	{
	  char *tmp = utf8_to_locale(dir->path);

	  screen_status_printf(_("Adding \'%s\' to playlist\n"), tmp);
	  g_free(tmp);
	}
#endif
      return 0;
    }

  if( entry->entity->type!=MPD_INFO_ENTITY_TYPE_SONG )
    return -1; 

  if( entry->flags & HIGHLIGHT )
    entry->flags &= ~HIGHLIGHT;
  else
    entry->flags |= HIGHLIGHT;

  if( entry->flags & HIGHLIGHT )
    {
      if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entry->entity->info.song;

	  if( mpdclient_cmd_add(c, song) == 0 )
	    {
	      char buf[BUFSIZE];
	      
	      strfsong(buf, BUFSIZE, LIST_FORMAT, song);
	      screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
	    }
	}
    }
  else
    {
      /* remove song from playlist */
      if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entry->entity->info.song;

	  if( song )
	    {
	      int index = playlist_get_index_from_file(c, song->file);
	      
	      while( (index=playlist_get_index_from_file(c, song->file))>=0 )
		mpdclient_cmd_delete(c, index);
	    }
	}
    }
  return 0;
}

static void
browse_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
}

static void
browse_resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}

static void
browse_exit(void)
{
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
  list_window_free(lw);
}

static void 
browse_open(screen_t *screen, mpdclient_t *c)
{
  if( filelist == NULL )
    {
      filelist = mpdclient_filelist_get(c, "");
      mpdclient_install_playlist_callback(c, playlist_changed_callback);
      mpdclient_install_browse_callback(c, file_changed_callback);
    }
}

static void
browse_close(void)
{
}

static char *
browse_title(char *str, size_t size)
{
  snprintf(str, size, _("Browse: %s"), basename(filelist->path));
  return str;
}

static void 
browse_paint(screen_t *screen, mpdclient_t *c)
{
  lw->clear = 1;
  
  list_window_paint(lw, list_callback, (void *) c);
  wnoutrefresh(lw->w);
}

static void 
browse_update(screen_t *screen, mpdclient_t *c)
{
  if( filelist->updated )
    {
      browse_paint(screen, c);
      filelist->updated = FALSE;
      return;
    }
  list_window_paint(lw, list_callback, (void *) c);
  wnoutrefresh(lw->w);
}


static int 
browse_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
  switch(cmd)
    {
    case CMD_PLAY:
      handle_enter(screen, c);
      return 1;
    case CMD_SELECT:
      if( handle_select(screen, c) == 0 )
	{
	  /* continue and select next item... */
	  cmd = CMD_LIST_NEXT;
	}
      break;
    case CMD_DELETE:
      handle_delete(screen, c);
      break;
    case CMD_SCREEN_UPDATE:
      filelist = mpdclient_filelist_update(c, filelist);
      list_window_check_selected(lw, filelist->length);
      screen_status_printf(_("Screen updated!"));
      return 1;
    case CMD_DB_UPDATE:
      if( !c->status->updatingDb )
	{
	  if( mpdclient_cmd_db_update(c,filelist->path)==0 )
	    {
	      screen_status_printf(_("Database update of %s started!"),
				   filelist->path);
	      /* set updatingDb to make shure the browse callback gets called
	       * even if the updated has finished before status is updated */
	      c->status->updatingDb = 1; 
	    }
	}
      else
	screen_status_printf(_("Database update running..."));
      return 1;
    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      return screen_find(screen, c, 
			 lw, filelist->length,
			 cmd, list_callback);
    default:
      break;
    }
  return list_window_cmd(lw, filelist->length, cmd);
}


list_window_t *
get_filelist_window()
{
  return lw;
}




screen_functions_t *
get_screen_browse(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = browse_init;
  functions.exit   = browse_exit;
  functions.open   = browse_open;
  functions.close  = browse_close;
  functions.resize = browse_resize;
  functions.paint  = browse_paint;
  functions.update = browse_update;
  functions.cmd    = browse_cmd;
  functions.get_lw = get_filelist_window;
  functions.get_title = browse_title;

  return &functions;
}

