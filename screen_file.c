#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "support.h"
#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_file.h"

#define BUFSIZE 1024

#undef USE_OLD_LAYOUT

static char *
list_callback(int index, int *highlight, void *data)
{
  static char buf[BUFSIZE];
  mpd_client_t *c = (mpd_client_t *) data;
  filelist_entry_t *entry;
  mpd_InfoEntity *entity;

  *highlight = 0;
  if( (entry=(filelist_entry_t *) g_list_nth_data(c->filelist, index))==NULL )
    return NULL;

  entity = entry->entity;
  *highlight = entry->selected;

  if( entity == NULL )
    {
#ifdef USE_OLD_LAYOUT
      return "[..]";
#else
      return "<d> ..";
#endif
    }
  if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) 
    {
      mpd_Directory *dir = entity->info.directory;
      char *dirname = utf8_to_locale(basename(dir->path));

#ifdef USE_OLD_LAYOUT
      snprintf(buf, BUFSIZE, "[%s]", dirname);
#else
      snprintf(buf, BUFSIZE, "<d> %s", dirname);
#endif
      free(dirname);
      return buf;
    }
  else if( entity->type==MPD_INFO_ENTITY_TYPE_SONG )
    {
      mpd_Song *song = entity->info.song;

#ifdef USE_OLD_LAYOUT      
      return mpc_get_song_name(song));
#else
      snprintf(buf, BUFSIZE, "<m> %s", mpc_get_song_name(song));
      return buf;
#endif

    }
  else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    {
      mpd_PlaylistFile *plf = entity->info.playlistFile;
      char *filename = utf8_to_locale(basename(plf->path));

#ifdef USE_OLD_LAYOUT      
      snprintf(buf, BUFSIZE, "*%s*", filename);
#else      
      snprintf(buf, BUFSIZE, "<p> %s", filename);
#endif
      free(filename);
      return buf;
    }
  return "Error: Unknow entry!";
}

static int
change_directory(screen_t *screen, mpd_client_t *c, filelist_entry_t *entry)
{
  list_window_t *w = screen->filelist;
  mpd_InfoEntity *entity = entry->entity;

  if( entity==NULL )
    {
      char *parent = g_path_get_dirname(c->cwd);

      if( strcmp(parent,".") == 0 )
	{
	  parent[0] = '\0';
	}
      if( c->cwd )
	free(c->cwd);
      c->cwd = parent;
    }
  else
    if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY)
      {
	mpd_Directory *dir = entity->info.directory;
	if( c->cwd )
	  free(c->cwd);
	c->cwd = strdup(dir->path);      
      }
    else
      return -1;
  
  mpc_update_filelist(c);
  list_window_reset(w);
  return 0;
}

static int
load_playlist(screen_t *screen, mpd_client_t *c, filelist_entry_t *entry)
{
  mpd_InfoEntity *entity = entry->entity;
  mpd_PlaylistFile *plf = entity->info.playlistFile;
  char *filename = utf8_to_locale(basename(plf->path));

  mpd_sendLoadCommand(c->connection, plf->path);
  mpd_finishCommand(c->connection);

  screen_status_printf("Loading playlist %s...", filename);
  free(filename);
  return 0;
}

static int 
handle_delete(screen_t *screen, mpd_client_t *c)
{
  list_window_t *lw = screen->filelist;
  filelist_entry_t *entry;
  mpd_InfoEntity *entity;
  mpd_PlaylistFile *plf;
  char *str, buf[BUFSIZE];
  int key;

  entry = ( filelist_entry_t *) g_list_nth_data(c->filelist, lw->selected);
  if( entry==NULL || entry->entity==NULL )
    return -1;

  entity = entry->entity;

  if( entity->type!=MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    {
      screen_status_printf("You can only delete playlists!");
      beep();
      return -1;
    }

  plf = entity->info.playlistFile;
  str = utf8_to_locale(basename(plf->path));
  snprintf(buf, BUFSIZE, "Delete playlist %s [y/n] ? ", str);
  free(str);  
  key = tolower(screen_getch(screen->status_window.w, buf));
  if( key!='y' )
    {
      screen_status_printf("Aborted!");
      return 0;
    }

  mpd_sendRmCommand(c->connection, plf->path);
  mpd_finishCommand(c->connection);
  if( mpc_error(c))
    {
      str = utf8_to_locale(mpc_error_str(c));
      screen_status_printf("Error: %s", str);
      free(str);
      beep();
      return -1;
    }
  screen_status_printf("Playlist deleted!");
  mpc_update_filelist(c);
  list_window_check_selected(lw, c->filelist_length);
  return 0;
}


static int
handle_play_cmd(screen_t *screen, mpd_client_t *c)
{
  list_window_t *w = screen->filelist;
  filelist_entry_t *entry;
  mpd_InfoEntity *entity;
  
  entry = ( filelist_entry_t *) g_list_nth_data(c->filelist, w->selected);
  if( entry==NULL )
    return -1;

  entity = entry->entity;
  if( entity==NULL || entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY )
    return change_directory(screen, c, entry);
  else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
    return load_playlist(screen, c, entry);
  return -1;
}

static int
add_directory(mpd_client_t *c, char *dir)
{
  mpd_InfoEntity *entity;
  GList *subdir_list = NULL;
  GList *list = NULL;
  char *dirname;

  dirname = utf8_to_locale(dir);
  screen_status_printf("Adding directory %s...\n", dirname);
  free(dirname);
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
  mpd_finishCommand(c->connection);
  
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

static int
handle_select(screen_t *screen, mpd_client_t *c)
{
  list_window_t *w = screen->filelist;
  filelist_entry_t *entry;

  entry = ( filelist_entry_t *) g_list_nth_data(c->filelist, w->selected);
  if( entry==NULL || entry->entity==NULL)
    return -1;

  if( entry->entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY )
    {
      mpd_Directory *dir = entry->entity->info.directory;
      add_directory(c, dir->path);
      return 0;
    }

  if( entry->entity->type!=MPD_INFO_ENTITY_TYPE_SONG )
    return -1; 

  entry->selected = !entry->selected;

  if( entry->selected )
    {
      if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entry->entity->info.song;

	  mpd_sendAddCommand(c->connection, song->file);
	  mpd_finishCommand(c->connection);

	  screen_status_printf("Adding \'%s\' to playlist\n", 
			       mpc_get_song_name(song));
	}
    }
  else
    {
      if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  int i;
	  mpd_Song *song = entry->entity->info.song;
	  
	  i = mpc_playlist_get_song_index(c, song->file);
	  if( i>=0 )
	    {
	      mpd_sendDeleteCommand(c->connection, i);
	      mpd_finishCommand(c->connection);
	      screen_status_printf("Removed \'%s\' from playlist\n", 
				   mpc_get_song_name(song));

	    }
	}
    }
  return 0;
}

void
file_clear_highlights(mpd_client_t *c)
{
  GList *list = g_list_first(c->filelist);
  
  while( list )
    {
      filelist_entry_t *entry = list->data;

      entry->selected = 0;
      list = list->next;
    }
}

void
file_clear_highlight(mpd_client_t *c, mpd_Song *song)
{
  GList *list = g_list_first(c->filelist);

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
	      entry->selected = 0;
	    }
	}
      list = list->next;
    }
}

char *
file_get_header(mpd_client_t *c)
{
  static char buf[BUFSIZE];
  char *tmp;

  tmp = utf8_to_locale(basename(c->cwd));
  snprintf(buf, BUFSIZE, 
	   TOP_HEADER_FILE ": %s                          ",
	   tmp
	   );
  free(tmp);

  return buf;
}

void 
file_open(screen_t *screen, mpd_client_t *c)
{
  if( c->filelist == NULL )
    {
      mpc_update_filelist(c);
    }
}

void 
file_close(screen_t *screen, mpd_client_t *c)
{
}

void 
file_paint(screen_t *screen, mpd_client_t *c)
{
  list_window_t *w = screen->filelist;
 
  w->clear = 1;
  
  list_window_paint(screen->filelist, list_callback, (void *) c);
  wnoutrefresh(screen->filelist->w);
}

void 
file_update(screen_t *screen, mpd_client_t *c)
{
  if( c->filelist_updated )
    {
      file_paint(screen, c);
      c->filelist_updated = 0;
      return;
    }
  list_window_paint(screen->filelist, list_callback, (void *) c);
  wnoutrefresh(screen->filelist->w);
}


int 
file_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
  switch(cmd)
    {
    case CMD_PLAY:
      handle_play_cmd(screen, c);
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
      mpc_update_filelist(c);
      list_window_check_selected(screen->filelist, c->filelist_length);
      screen_status_printf("Screen updated!");
      return 1;
    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      return screen_find(screen, c, 
			 screen->filelist, c->filelist_length,
			 cmd, list_callback);
    default:
      break;
    }
  return list_window_cmd(screen->filelist, c->filelist_length, cmd);
}
