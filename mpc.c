/* 
 * $Id: mpc.c,v 1.5 2004/03/17 23:19:21 kalle Exp $ 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "support.h"
#include "libmpdclient.h"
#include "mpc.h"
#include "options.h"

#define MAX_SONG_LENGTH 1024

void 
mpc_update_song(mpd_client_t *c)
{
  mpd_InfoEntity *entity;

  if( c->song )
    {
      mpd_freeSong(c->song);
      c->song = NULL;
    }

  mpd_sendPlaylistInfoCommand(c->connection, c->status->song);
  while( (entity=mpd_getNextInfoEntity(c->connection)) )
    {
      mpd_Song *song = entity->info.song;

      if(c->connection->error) 
	{
	  fprintf(stderr,"error: %s\n",c->connection->errorStr);
	  exit(EXIT_FAILURE);
	}
      if(entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
	mpd_freeInfoEntity(entity);
	fprintf(stderr,
		"error: type != MPD_INFO_ENTITY_TYPE_SONG [%d]\n",
		entity->type);
	exit(EXIT_FAILURE);
      }
      c->song = mpd_songDup(song);
      mpd_freeInfoEntity(entity);
    }
}

int 
mpc_close(mpd_client_t *c)
{
  if( c->connection )
    mpd_closeConnection(c->connection);
  if( c->cwd )
    free( c->cwd );
  
  return 0;
}

mpd_client_t *
mpc_connect(char *host, int port)
{
  mpd_Connection *connection;
  mpd_client_t *c;

  connection =  mpd_newConnection(host, port, 10);
  if( connection==NULL )
    {
      fprintf(stderr, "mpd_newConnection to %s:%d failed!\n", host, port);
      exit(EXIT_FAILURE);
    }
  
  c = malloc(sizeof(mpd_client_t));
  memset(c, 0, sizeof(mpd_client_t));
  c->connection = connection;
  c->cwd = strdup("");

  return c;
}

int
mpc_error(mpd_client_t *c)
{
  if( c == NULL || c->connection == NULL )
    return 1;
  if( c->connection->error )
    return 1;

  return 0;
}

char *
mpc_error_str(mpd_client_t *c)
{
  if( c == NULL || c->connection == NULL )
    return "Not connected";

  if( c->connection && c->connection->errorStr )
    return c->connection->errorStr;

  return NULL;
}



int
mpc_free_playlist(mpd_client_t *c)
{
  GList *list;

  if( c==NULL || c->playlist==NULL )
    return -1;

  list=g_list_first(c->playlist);

  while( list!=NULL )
    {
      mpd_Song *song = (mpd_Song *) list->data;

      mpd_freeSong(song);
      list=list->next;
    }
  g_list_free(c->playlist);
  c->playlist=NULL;
  c->playlist_length=0;

  return 0;
}

int 
mpc_update_playlist(mpd_client_t *c)
{
  mpd_InfoEntity *entity;

  //  fprintf(stderr, "mpc_update_playlist(): status->playlist = %d\n",  c->status->playlist);

  if( c->playlist )
    mpc_free_playlist(c);

  c->playlist_length=0;
  mpd_sendPlaylistInfoCommand(c->connection,-1);
  while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
    {
      if(entity->type==MPD_INFO_ENTITY_TYPE_SONG) 
	{
	  mpd_Song *song = mpd_songDup(entity->info.song);

	  c->playlist = g_list_append(c->playlist, (gpointer) song);
	  c->playlist_length++;
	}
      mpd_freeInfoEntity(entity);
    }
  c->playlist_id = c->status->playlist;
  c->playlist_updated = 1;
  c->song_id = -1;

  return 0;
}

int
mpc_playlist_get_song_index(mpd_client_t *c, char *filename)
{
  GList *list = c->playlist;
  int i=0;

  while( list )
    {
      mpd_Song *song = (mpd_Song *) list->data;
      if( strcmp(song->file, filename ) == 0 )	
	return i;
      list=list->next;
      i++;
    }
  return -1;
}

mpd_Song *
mpc_playlist_get_song(mpd_client_t *c, int n)
{
  return (mpd_Song *) g_list_nth_data(c->playlist, n);
}

char *
mpc_get_song_name(mpd_Song *song)
{
  static char buf[MAX_SONG_LENGTH];
  char *name;
  
  if( song->title )
    {
      if( song->artist )
	{
	  snprintf(buf, MAX_SONG_LENGTH, "%s - %s", song->artist, song->title);
	  name = utf8_to_locale(buf);
	  strncpy(buf, name, MAX_SONG_LENGTH);
	  free(name);
	  return buf;
	}
      else
	{
	  name = utf8_to_locale(song->title);
	  strncpy(buf, name, MAX_SONG_LENGTH);
	  free(name);
	  return buf;
	}
    }
  name = utf8_to_locale(song->file);
  strncpy(buf, name, MAX_SONG_LENGTH);
  free(name);
  return buf;
}

int 
mpc_update(mpd_client_t *c)
{
  if( c->status )
    {
      mpd_freeStatus(c->status);
    }

  c->status = mpd_getStatus(c->connection);
  
  //  if( c->playlist == NULL || c->playlist_id!=c->status->playlist )
  if( c->playlist_id!=c->status->playlist )
    mpc_update_playlist(c);
  
  //  if( c->song == NULL || c->status->song != c->song_id )
  if( c->status->song != c->song_id )
    {
      c->song = mpc_playlist_get_song(c, c->status->song);
      c->song_id = c->status->song;
      c->song_updated = 1;
    }

  return 0;
}






int
mpc_free_filelist(mpd_client_t *c)
{
  GList *list;

  if( c==NULL || c->filelist==NULL )
    return -1;

  list=g_list_first(c->filelist);

  while( list!=NULL )
    {
      filelist_entry_t *entry = list->data;

      if( entry->entity )
	mpd_freeInfoEntity(entry->entity);
      free(entry);
      list=list->next;
    }
  g_list_free(c->filelist);
  c->filelist=NULL;
  c->filelist_length=0;

  return 0;
}



int 
mpc_update_filelist(mpd_client_t *c)
{
  mpd_InfoEntity *entity;

  if( c->filelist )
    mpc_free_filelist(c);

  c->filelist_length=0;

  //  mpd_sendListallCommand(conn,"");
  mpd_sendLsInfoCommand(c->connection, c->cwd);

  if( c->cwd && c->cwd[0] )
    {
      /* add a dummy entry for ./.. */
      filelist_entry_t *entry = malloc(sizeof(filelist_entry_t));
      memset(entry, 0, sizeof(filelist_entry_t));
      entry->entity = NULL;
      c->filelist = g_list_append(c->filelist, (gpointer) entry);
      c->filelist_length++;
    }

  while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
    {
      filelist_entry_t *entry = malloc(sizeof(filelist_entry_t));
      
      memset(entry, 0, sizeof(filelist_entry_t));
      entry->entity = entity;
      c->filelist = g_list_append(c->filelist, (gpointer) entry);
      c->filelist_length++;
    }
  
  c->filelist_updated = 1;

  mpd_finishCommand(c->connection);

  mpc_filelist_set_selected(c);

  return 0;
}

int 
mpc_filelist_set_selected(mpd_client_t *c)
{
  GList *list = c->filelist;

  while( list )
    {
      filelist_entry_t *entry = list->data;
      mpd_InfoEntity *entity = entry->entity ;      
      
      if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG )
	{
	  mpd_Song *song = entity->info.song;

	  if( mpc_playlist_get_song_index(c, song->file) >= 0 )
	    entry->selected = 1;
	  else
	    entry->selected = 0;
	}

      list=list->next;
    }
  return 0;
}
