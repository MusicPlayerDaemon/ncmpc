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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "libmpdclient.h"
#include "mpc.h"
#include "options.h"

#define MAX_SONG_LENGTH 1024

int 
mpc_close(mpd_client_t *c)
{
  if( c->connection )
    mpd_closeConnection(c->connection);
  if( c->cwd )
    g_free( c->cwd );
  
  return 0;
}

mpd_client_t *
mpc_connect(char *host, int port, char *password)
{
  mpd_Connection *connection;
  mpd_client_t *c;

  connection =  mpd_newConnection(host, port, 10);
  if( connection==NULL )
    {
      fprintf(stderr, "mpd_newConnection to %s:%d failed!\n", host, port);
      exit(EXIT_FAILURE);
    }
  
  c = g_malloc(sizeof(mpd_client_t));
  memset(c, 0, sizeof(mpd_client_t));
  c->connection = connection;
  c->cwd = g_strdup("");

  if( password )
    {
      mpd_sendPasswordCommand(connection, password);
      mpd_finishCommand(connection);
    }

  return c;
}

int
mpc_reconnect(mpd_client_t *c, char *host, int port, char *password)
{
  mpd_Connection *connection;

  connection =  mpd_newConnection(host, port, 10);
  if( connection==NULL )
    return -1;
  if( connection->error )
    {
      mpd_closeConnection(connection);
      return -1;
    }
  
  c->connection = connection;

  if( password )
    {
      mpd_sendPasswordCommand(connection, password);
      mpd_finishCommand(connection);
    }

  return 0;
}


int
mpc_error(mpd_client_t *c)
{
  if( c == NULL || c->connection == NULL )
    return 1;

  if( c->connection->error )
      return c->connection->error;

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

  c->song_id = -1;
  c->song = NULL;

  return 0;
}

int 
mpc_get_playlist(mpd_client_t *c)
{
  mpd_InfoEntity *entity;

  D(fprintf(stderr, "mpc_get_playlist() [%lld]\n", c->status->playlist));

  if( mpc_error(c) )
    return -1;

  if( c->playlist )
    mpc_free_playlist(c);

  c->playlist_length=0;
  mpd_sendPlaylistInfoCommand(c->connection,-1);
  if( mpc_error(c) )
    return -1;
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
  mpd_finishCommand(c->connection);
  c->playlist_id = c->status->playlist;
  c->playlist_updated = 1;
  c->song_id = -1;
  c->song = NULL;

  mpc_filelist_set_selected(c);

  return 0;
}

int 
mpc_update_playlist(mpd_client_t *c)
{
  mpd_InfoEntity *entity;

  D(fprintf(stderr, "mpc_update_playlist() [%lld -> %lld]\n", 
	    c->status->playlist, c->playlist_id));

  if( mpc_error(c) )
    return -1;

  mpd_sendPlChangesCommand(c->connection, c->playlist_id);
  if( mpc_error(c) )
    return -1;

  while( (entity=mpd_getNextInfoEntity(c->connection)) != NULL   ) 
    {
      if(entity->type==MPD_INFO_ENTITY_TYPE_SONG) 
	{
	  mpd_Song *song;
	  GList *item;

	  if( (song=mpd_songDup(entity->info.song)) == NULL )
	    {
	      D(fprintf(stderr, "song==NULL\n"));
	      return mpc_get_playlist(c);
	    }

	  item =  g_list_nth(c->playlist, song->num);
	  if( item && item->data)
	    {
	      /* Update playlist entry */
	      mpd_freeSong((mpd_Song *) item->data);
	      item->data = song;
	      if( c->song_id == song->num )
		c->song = song;
	      D(fprintf(stderr, "Changing num %d to %s\n",
			song->num, mpc_get_song_name(song)));
	    }
	  else
	    {
	      /* Add a new  playlist entry */
	      D(fprintf(stderr, "Adding num %d - %s\n",
			song->num, mpc_get_song_name(song)));
	      c->playlist = g_list_append(c->playlist, 
					  (gpointer) song);
	      c->playlist_length++;
	    }
	}
      mpd_freeInfoEntity(entity);      
    }
  mpd_finishCommand(c->connection);
  
  while( g_list_length(c->playlist) > c->status->playlistLength )
    {
      GList *item = g_list_last(c->playlist);

      /* Remove the last playlist entry */
      mpd_freeSong((mpd_Song *) item->data);
      c->playlist = g_list_delete_link(c->playlist, item);
      c->playlist_length--;      
      D(fprintf(stderr, "Removed the last playlist entryn\n"));
    }

  c->playlist_id = c->status->playlist;
  c->playlist_updated = 1;
  mpc_filelist_set_selected(c);

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
	  g_free(name);
	  return buf;
	}
      else
	{
	  name = utf8_to_locale(song->title);
	  strncpy(buf, name, MAX_SONG_LENGTH);
	  g_free(name);
	  return buf;
	}
    }
  name = utf8_to_locale(basename(song->file));
  strncpy(buf, name, MAX_SONG_LENGTH);
  g_free(name);
  return buf;
}

char *
mpc_get_song_name2(mpd_Song *song)
{
  static char buf[MAX_SONG_LENGTH];
  char *name;

  /* streams */
  if( song->name )
    {
      name = utf8_to_locale(song->name);
      strncpy(buf, name, MAX_SONG_LENGTH);
      g_free(name);
      return buf;
    }
  else if( strstr(song->file, "://") )
    {
      name = utf8_to_locale(song->file);
      strncpy(buf, name, MAX_SONG_LENGTH);
      g_free(name);
      
      return buf;
    }

  /* regular songs */
  if( song->title )
    {      
      if( song->artist )
	{
	  snprintf(buf, MAX_SONG_LENGTH, "%s - %s", song->artist, song->title);
	  name = utf8_to_locale(buf);
	  strncpy(buf, name, MAX_SONG_LENGTH);
	  g_free(name);
	  return buf;
	}
      else
	{
	  name = utf8_to_locale(song->title);
	  strncpy(buf, name, MAX_SONG_LENGTH);
	  g_free(name);
	  return buf;
	}
    }
  name = utf8_to_locale(basename(song->file));
  strncpy(buf, name, MAX_SONG_LENGTH);
  g_free(name);
  return buf;
}

#if 0
size_t
strfsong(char *s, size_t max, const char *format, mpd_Song *song)
{
  size_t i, len, format_len;
  char prev;

  void sappend(char *utfstr) {
    char *tmp = utf8_to_locale(utfstr);
    size_t tmplen = strlen(tmp);
    if( i+tmplen < max )
      strcat(s, tmp);
    else
      strncat(s, tmp, max-i);
    i = strlen(s);
    g_free(tmp);
  }

  i = 0;
  len = 0;
  format_len = strlen(format);
  memset(s, 0, max);
  while(i<format_len && len<max)
    {
      if( i>0 && format[i-1]=='%' )
	{
	  char *tmp;
	  size_t tmplen;

	  switch(format[i])
	    {
	    case '%':
	      s[len++] = format[i];
	      break;
	    case 'a':
	      sappend(song->artist);
	      break;
	    case 't':
	      sappend(song->title);
	      break;
	    case 'n':
	      sappend(song->name);
	      break;
	    case 'f':
	      sappend(song->file);
	      break;
	    }
	}
      else if( format[i]!='%' )
	{
	  s[len] = format[i++];
	}
      len++;
    }

  return len;
}
#endif


int 
mpc_update(mpd_client_t *c)
{
  if( mpc_error(c) )
    return -1;

  if( c->status )
    {
      mpd_freeStatus(c->status);
    }

  c->status = mpd_getStatus(c->connection);
  if( mpc_error(c) )
    return -1;

  if( c->playlist_id!=c->status->playlist )
    {
      if( c->playlist_length<2 )
	mpc_get_playlist(c);
      else
	mpc_update_playlist(c);
    }
  
  if( !c->song || c->status->song != c->song_id )
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
      g_free(entry);
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

  if( mpc_error(c) )
    return -1;

  if( c->filelist )
    mpc_free_filelist(c);

  c->filelist_length=0;

  mpd_sendLsInfoCommand(c->connection, c->cwd);
  
  if( c->cwd && c->cwd[0] )
    {
      /* add a dummy entry for ./.. */
      filelist_entry_t *entry = g_malloc(sizeof(filelist_entry_t));
      memset(entry, 0, sizeof(filelist_entry_t));
      entry->entity = NULL;
      c->filelist = g_list_append(c->filelist, (gpointer) entry);
      c->filelist_length++;
    }

  while( (entity=mpd_getNextInfoEntity(c->connection)) ) 
    {
      filelist_entry_t *entry = g_malloc(sizeof(filelist_entry_t));
      
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
