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
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "mpdclient.h"
#include "utils.h"
#include "strfsong.h"
#include "wreadln.h"
#include "command.h"
#include "colors.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_play.h"
#include "gcc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <ncurses.h>

#define MAX_SONG_LENGTH 512

typedef struct
{
  GList **list;
  GList **dir_list;
  screen_t *screen;
  mpdclient_t *c;
} completion_callback_data_t;

static list_window_t *lw = NULL;

static void
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
	D("screen_play.c> playlist_callback() [%d]\n", event);
	switch(event) {
	case PLAYLIST_EVENT_DELETE:
		break;
	case PLAYLIST_EVENT_MOVE:
		lw->selected = *((int *) data);
		if( lw->selected<lw->start )
			lw->start--;
		break;
	default:
		break;
	}
	/* make shure the playlist is repainted */
	lw->clear = 1;
	lw->repaint = 1;
	list_window_check_selected(lw, c->playlist.list->len);
}

static const char *
list_callback(unsigned idx, int *highlight, void *data)
{
	static char songname[MAX_SONG_LENGTH];
	mpdclient_t *c = (mpdclient_t *) data;
	mpd_Song *song;

	if (idx >= playlist_length(&c->playlist))
		return NULL;

	song = playlist_get(&c->playlist, idx);

	if( c->song && song->id==c->song->id && !IS_STOPPED(c->status->state) ) {
		*highlight = 1;
	}
	strfsong(songname, MAX_SONG_LENGTH, LIST_FORMAT, song);
	return songname;
}

static void
center_playing_item(mpdclient_t *c)
{
	unsigned length = c->playlist.list->len;
	unsigned offset = lw->selected - lw->start;
	int idx;

	if (!c->song || length<lw->rows || IS_STOPPED(c->status->state))
		return;

	/* try to center the song that are playing */
	idx = playlist_get_index(c, c->song);
	D("Autocenter song id:%d pos:%d index:%d\n", c->song->id,c->song->pos,idx);
	if (idx < 0)
		return;

	list_window_center(lw, length, idx);

	/* make sure the cursor is in the window */
	lw->selected = lw->start+offset;
	list_window_check_selected(lw, length);

	return;
}

static void
save_pre_completion_cb(GCompletion *gcmp, mpd_unused gchar *line, void *data)
{
	completion_callback_data_t *tmp = (completion_callback_data_t *)data;
	GList **list = tmp->list;
	mpdclient_t *c = tmp->c;

	if( *list == NULL ) {
		/* create completion list */
		*list = gcmp_list_from_path(c, "", NULL, GCMP_TYPE_PLAYLIST);
		g_completion_add_items(gcmp, *list);
	}
}

static void
save_post_completion_cb(mpd_unused GCompletion *gcmp, mpd_unused gchar *line,
			GList *items, void *data)
{
	completion_callback_data_t *tmp = (completion_callback_data_t *)data;
	screen_t *screen = tmp->screen;

	if( g_list_length(items)>=1 ) {
		screen_display_completion_list(screen, items);
		lw->clear = 1;
		lw->repaint = 1;
	}
}

int
playlist_save(screen_t *screen, mpdclient_t *c, char *name, char *defaultname)
{
  gchar *filename;
  gint error;
  GCompletion *gcmp;
  GList *list = NULL;
  completion_callback_data_t data;

  if( name==NULL )
    {
      /* initialize completion support */
      gcmp = g_completion_new(NULL);
      g_completion_set_compare(gcmp, strncmp);
      data.list = &list;
      data.dir_list = NULL;
      data.screen = screen;
      data.c = c;
      wrln_completion_callback_data = &data;
      wrln_pre_completion_callback = save_pre_completion_cb;
      wrln_post_completion_callback = save_post_completion_cb;


      /* query the user for a filename */
      filename = screen_readln(screen->status_window.w,
			       _("Save playlist as: "),
			       defaultname,
			       NULL,
			       gcmp);     			       

      /* destroy completion support */
      wrln_completion_callback_data = NULL;
      wrln_pre_completion_callback = NULL;
      wrln_post_completion_callback = NULL;
      g_completion_free(gcmp);
      list = string_list_free(list);
      if( filename )
	filename=g_strstrip(filename);
    }
  else
    {
      filename=g_strdup(name);
    }
  if( filename==NULL || filename[0]=='\0' )
    return -1;
  /* send save command to mpd */
  D("Saving playlist as \'%s \'...\n", filename);
  if( (error=mpdclient_cmd_save_playlist(c, filename)) )
    {
      gint code = GET_ACK_ERROR_CODE(error);

      if( code == MPD_ACK_ERROR_EXIST )
	{
	  char *buf;
	  int key;

	  buf=g_strdup_printf(_("Replace %s [%s/%s] ? "), filename, YES, NO);
	  key = tolower(screen_getch(screen->status_window.w, buf));
	  g_free(buf);
	  if( key == YES[0] )
	    {
	      if( mpdclient_cmd_delete_playlist(c, filename) )
		{
		  g_free(filename);
		  return -1;
		}
	      error = playlist_save(screen, c, filename, NULL);
	      g_free(filename);
	      return error;
	    }	  
	  screen_status_printf(_("Aborted!"));
	}
      g_free(filename);
      return -1;
    }
  /* success */
  screen_status_printf(_("Saved %s"), filename);
  g_free(filename);
  return 0;
}

static void add_dir(GCompletion *gcmp, gchar *dir, GList **dir_list,
                    GList **list, mpdclient_t *c)
{
  g_completion_remove_items(gcmp, *list);
  *list = string_list_remove(*list, dir);
  *list = gcmp_list_from_path(c, dir, *list, GCMP_TYPE_RFILE);
  g_completion_add_items(gcmp, *list);
  *dir_list = g_list_append(*dir_list, g_strdup(dir));
}

static void add_pre_completion_cb(GCompletion *gcmp, gchar *line, void *data)
{
  completion_callback_data_t *tmp = (completion_callback_data_t *)data;
  GList **dir_list = tmp->dir_list;
  GList **list = tmp->list;
  mpdclient_t *c = tmp->c;

  D("pre_completion()...\n");
  if( *list == NULL )
    {
      /* create initial list */
      *list = gcmp_list_from_path(c, "", NULL, GCMP_TYPE_RFILE);
      g_completion_add_items(gcmp, *list);
    }
  else if( line && line[0] && line[strlen(line)-1]=='/' &&
	   string_list_find(*dir_list, line) == NULL )
    {	  
      /* add directory content to list */
      add_dir(gcmp, line, dir_list, list, c);
    }
}

static void add_post_completion_cb(GCompletion *gcmp, gchar *line,
                                   GList *items, void *data)
{
  completion_callback_data_t *tmp = (completion_callback_data_t *)data;
  GList **dir_list = tmp->dir_list;
  GList **list = tmp->list;
  mpdclient_t *c = tmp->c;
  screen_t *screen = tmp->screen;

  D("post_completion()...\n");
  if( g_list_length(items)>=1 )
    {
      screen_display_completion_list(screen, items);
      lw->clear = 1;
      lw->repaint = 1;
    }

  if( line && line[0] && line[strlen(line)-1]=='/' &&
      string_list_find(*dir_list, line) == NULL )
    {	  
      /* add directory content to list */
      add_dir(gcmp, line, dir_list, list, c);
    }
}

static int
handle_add_to_playlist(screen_t *screen, mpdclient_t *c)
{
  gchar *path;
  GCompletion *gcmp;
  GList *list = NULL;
  GList *dir_list = NULL;
  completion_callback_data_t data;
    
  /* initialize completion support */
  gcmp = g_completion_new(NULL);
  g_completion_set_compare(gcmp, strncmp);
  data.list = &list;
  data.dir_list = &dir_list;
  data.screen = screen;
  data.c = c;
  wrln_completion_callback_data = &data;
  wrln_pre_completion_callback = add_pre_completion_cb;
  wrln_post_completion_callback = add_post_completion_cb;
  /* get path */
  path = screen_readln(screen->status_window.w, 
		       _("Add: "),
		       NULL, 
		       NULL, 
		       gcmp);

  /* destroy completion data */
  wrln_completion_callback_data = NULL;
  wrln_pre_completion_callback = NULL;
  wrln_post_completion_callback = NULL;
  g_completion_free(gcmp);
  string_list_free(list);
  string_list_free(dir_list);

  /* add the path to the playlist */
  if( path && path[0] )
    mpdclient_cmd_add_path(c, path);

  return 0;
}

static void
play_init(WINDOW *w, int cols, int rows)
{
	lw = list_window_init(w, cols, rows);
}

static void
play_open(mpd_unused screen_t *screen, mpdclient_t *c)
{
	static gboolean install_cb = TRUE;

	if (install_cb) {
		mpdclient_install_playlist_callback(c, playlist_changed_callback);
		install_cb = FALSE;
	}
}

static void
play_resize(int cols, int rows)
{
	lw->cols = cols;
	lw->rows = rows;
}


static void
play_exit(void)
{
	list_window_free(lw);
}

static const char *
play_title(char *str, size_t size)
{
	if( strcmp(options.host, "localhost") == 0 )
		return _("Playlist");

	g_snprintf(str, size, _("Playlist on %s"), options.host);
	return str;
}

static void
play_paint(mpd_unused screen_t *screen, mpdclient_t *c)
{
	lw->clear = 1;

	list_window_paint(lw, list_callback, (void *) c);
	wnoutrefresh(lw->w);
}

static void
play_update(screen_t *screen, mpdclient_t *c)
{
	/* hide the cursor when mpd are playing and the user are inactive */
	if( options.hide_cursor>0 && c->status->state == MPD_STATUS_STATE_PLAY &&
	    time(NULL)-screen->input_timestamp >= options.hide_cursor ) {
		lw->flags |= LW_HIDE_CURSOR;
	} else {
		lw->flags &= ~LW_HIDE_CURSOR;
	}

	/* center the cursor */
	if( options.auto_center ) {
		static int prev_song_id = 0;

		if( c->song && prev_song_id != c->song->id ) {
			center_playing_item(c);
			prev_song_id = c->song->id;
		}
	}

	if( c->playlist.updated ) {
		if (lw->selected >= c->playlist.list->len)
			lw->selected = c->playlist.list->len - 1;
		if (lw->start >= c->playlist.list->len)
			list_window_reset(lw);

		play_paint(screen, c);
		c->playlist.updated = FALSE;
	} else if( lw->repaint || 1) {
		list_window_paint(lw, list_callback, (void *) c);
		wnoutrefresh(lw->w);
		lw->repaint = 0;
	}
}

#ifdef HAVE_GETMOUSE
static int
handle_mouse_event(mpd_unused screen_t *screen, mpdclient_t *c)
{
	int row;
	unsigned selected;
	unsigned long bstate;

	if (screen_get_mouse_event(c, lw, c->playlist.list->len, &bstate, &row))
		return 1;

	if (bstate & BUTTON1_DOUBLE_CLICKED) {
		/* stop */
		screen_cmd(c, CMD_STOP);
		return 1;
	}

	selected = lw->start + row;

	if (bstate & BUTTON1_CLICKED) {
		/* play */
		if (lw->start + row < c->playlist.list->len)
			mpdclient_cmd_play(c, lw->start + row);
	} else if (bstate & BUTTON3_CLICKED) {
		/* delete */
		if (selected == lw->selected)
			mpdclient_cmd_delete(c, lw->selected);
	}

	lw->selected = selected;
	list_window_check_selected(lw, c->playlist.list->len);

	return 1;
}
#else
#define handle_mouse_event(s,c) (0)
#endif

static int
play_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
	switch(cmd) {
	case CMD_PLAY:
		mpdclient_cmd_play(c, lw->selected);
		return 1;
	case CMD_DELETE:
		mpdclient_cmd_delete(c, lw->selected);
		return 1;
	case CMD_SAVE_PLAYLIST:
		playlist_save(screen, c, NULL, NULL);
		return 1;
	case CMD_ADD:
		handle_add_to_playlist(screen, c);
		return 1;
	case CMD_SCREEN_UPDATE:
		screen->painted = 0;
		lw->clear = 1;
		lw->repaint = 1;
		center_playing_item(c);
		return 1;
	case CMD_LIST_MOVE_UP:
		mpdclient_cmd_move(c, lw->selected, lw->selected-1);
		return 1;
	case CMD_LIST_MOVE_DOWN:
		mpdclient_cmd_move(c, lw->selected, lw->selected+1);
		return 1;
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		return screen_find(screen,
				   lw, c->playlist.list->len,
				   cmd, list_callback, (void *) c);
	case CMD_MOUSE_EVENT:
		return handle_mouse_event(screen,c);
	default:
		break;
	}
	return list_window_cmd(lw, c->playlist.list->len, cmd);
}

const struct screen_functions screen_playlist = {
	.init = play_init,
	.exit = play_exit,
	.open = play_open,
	.close = NULL,
	.resize = play_resize,
	.paint = play_paint,
	.update = play_update,
	.cmd = play_cmd,
	.get_title = play_title,
};
