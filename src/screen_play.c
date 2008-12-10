/*
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
#include "i18n.h"
#include "charset.h"
#include "options.h"
#include "mpdclient.h"
#include "utils.h"
#include "strfsong.h"
#include "wreadln.h"
#include "command.h"
#include "colors.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_play.h"

#ifndef NCMPC_MINI
#include "hscroll.h"
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>

#define MAX_SONG_LENGTH 512

#ifndef NCMPC_MINI
typedef struct
{
	GList **list;
	GList **dir_list;
	mpdclient_t *c;
} completion_callback_data_t;
#endif

static struct mpdclient_playlist *playlist;
static int current_song_id = -1;
static list_window_t *lw = NULL;
static guint timer_hide_cursor_id;

static void
play_paint(void);

static void
playlist_repaint(void)
{
	play_paint();
	wrefresh(lw->w);
}

static void
playlist_repaint_if_active(void)
{
	if (screen_is_visible(&screen_playlist))
		playlist_repaint();
}

static void
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
	switch (event) {
	case PLAYLIST_EVENT_DELETE:
		break;
	case PLAYLIST_EVENT_MOVE:
		lw->selected = *((int *) data);
		if (lw->selected < lw->start)
			lw->start--;
		break;
	default:
		break;
	}

	list_window_check_selected(lw, c->playlist.list->len);
	playlist_repaint_if_active();
}

static const char *
list_callback(unsigned idx, bool *highlight, G_GNUC_UNUSED void *data)
{
	static char songname[MAX_SONG_LENGTH];
#ifndef NCMPC_MINI
	static scroll_state_t st;
#endif
	mpd_Song *song;

	if (playlist == NULL || idx >= playlist_length(playlist))
		return NULL;

	song = playlist_get(playlist, idx);
	if (song->id == current_song_id)
		*highlight = true;

	strfsong(songname, MAX_SONG_LENGTH, options.list_format, song);

#ifndef NCMPC_MINI
	if (options.scroll && (unsigned)song->pos == lw->selected &&
	    utf8_width(songname) > (unsigned)COLS) {
		static unsigned current_song;
		char *tmp;

		if (current_song != lw->selected) {
			st.offset = 0;
			current_song = lw->selected;
		}

		tmp = strscroll(songname, options.scroll_sep,
				MAX_SONG_LENGTH, &st);
		g_strlcpy(songname, tmp, MAX_SONG_LENGTH);
		g_free(tmp);
	} else if ((unsigned)song->pos == lw->selected)
		st.offset = 0;
#endif

	return songname;
}

static void
center_playing_item(mpdclient_t *c)
{
	unsigned length = c->playlist.list->len;
	unsigned offset = lw->selected - lw->start;
	int idx;

	if (!c->song || length < lw->rows ||
	    c->status == NULL || IS_STOPPED(c->status->state))
		return;

	/* try to center the song that are playing */
	idx = playlist_get_index(c, c->song);
	if (idx < 0)
		return;

	list_window_center(lw, length, idx);

	/* make sure the cursor is in the window */
	lw->selected = lw->start+offset;
	list_window_check_selected(lw, length);
}

#ifndef NCMPC_MINI
static void
save_pre_completion_cb(GCompletion *gcmp, G_GNUC_UNUSED gchar *line,
		       void *data)
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
save_post_completion_cb(G_GNUC_UNUSED GCompletion *gcmp,
			G_GNUC_UNUSED gchar *line, GList *items,
			G_GNUC_UNUSED void *data)
{
	if (g_list_length(items) >= 1)
		screen_display_completion_list(items);
}
#endif

#ifndef NCMPC_MINI
/**
 * Wrapper for strncmp().  We are not allowed to pass &strncmp to
 * g_completion_set_compare(), because strncmp() takes size_t where
 * g_completion_set_compare passes a gsize value.
 */
static gint
completion_strncmp(const gchar *s1, const gchar *s2, gsize n)
{
	return strncmp(s1, s2, n);
}
#endif

int
playlist_save(mpdclient_t *c, char *name, char *defaultname)
{
	gchar *filename, *filename_utf8;
	gint error;
#ifndef NCMPC_MINI
	GCompletion *gcmp;
	GList *list = NULL;
	completion_callback_data_t data;
#endif

#ifdef NCMPC_MINI
	(void)defaultname;
#endif

#ifndef NCMPC_MINI
	if (name == NULL) {
		/* initialize completion support */
		gcmp = g_completion_new(NULL);
		g_completion_set_compare(gcmp, completion_strncmp);
		data.list = &list;
		data.dir_list = NULL;
		data.c = c;
		wrln_completion_callback_data = &data;
		wrln_pre_completion_callback = save_pre_completion_cb;
		wrln_post_completion_callback = save_post_completion_cb;


		/* query the user for a filename */
		filename = screen_readln(screen.status_window.w,
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
	} else
#endif
			filename=g_strdup(name);

	if (filename == NULL)
		return -1;

	/* send save command to mpd */

	filename_utf8 = locale_to_utf8(filename);
	error = mpdclient_cmd_save_playlist(c, filename_utf8);
	g_free(filename_utf8);

	if (error) {
		gint code = GET_ACK_ERROR_CODE(error);

		if (code == MPD_ACK_ERROR_EXIST) {
			char *buf;
			int key;

			buf = g_strdup_printf(_("Replace %s [%s/%s] ? "),
					      filename, YES, NO);
			key = tolower(screen_getch(screen.status_window.w,
						   buf));
			g_free(buf);

			if (key == YES[0]) {
				filename_utf8 = locale_to_utf8(filename);
				error = mpdclient_cmd_delete_playlist(c, filename_utf8);
				g_free(filename_utf8);

				if (error) {
					g_free(filename);
					return -1;
				}

				error = playlist_save(c, filename, NULL);
				g_free(filename);
				return error;
			}

			screen_status_printf(_("Aborted"));
		}

		g_free(filename);
		return -1;
	}

	/* success */
	screen_status_printf(_("Saved %s"), filename);
	g_free(filename);
	return 0;
}

#ifndef NCMPC_MINI
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

	if (*list == NULL) {
		/* create initial list */
		*list = gcmp_list_from_path(c, "", NULL, GCMP_TYPE_RFILE);
		g_completion_add_items(gcmp, *list);
	} else if (line && line[0] && line[strlen(line)-1]=='/' &&
		   string_list_find(*dir_list, line) == NULL) {
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

	if (g_list_length(items) >= 1)
		screen_display_completion_list(items);

	if (line && line[0] && line[strlen(line) - 1] == '/' &&
	    string_list_find(*dir_list, line) == NULL) {
		/* add directory content to list */
		add_dir(gcmp, line, dir_list, list, c);
	}
}
#endif

static int
handle_add_to_playlist(mpdclient_t *c)
{
	gchar *path;
#ifndef NCMPC_MINI
	GCompletion *gcmp;
	GList *list = NULL;
	GList *dir_list = NULL;
	completion_callback_data_t data;

	/* initialize completion support */
	gcmp = g_completion_new(NULL);
	g_completion_set_compare(gcmp, completion_strncmp);
	data.list = &list;
	data.dir_list = &dir_list;
	data.c = c;
	wrln_completion_callback_data = &data;
	wrln_pre_completion_callback = add_pre_completion_cb;
	wrln_post_completion_callback = add_post_completion_cb;
#endif

	/* get path */
	path = screen_readln(screen.status_window.w,
			     _("Add: "),
			     NULL,
			     NULL,
#ifdef NCMPC_MINI
			     NULL
#else
			     gcmp
#endif
			     );

	/* destroy completion data */
#ifndef NCMPC_MINI
	wrln_completion_callback_data = NULL;
	wrln_pre_completion_callback = NULL;
	wrln_post_completion_callback = NULL;
	g_completion_free(gcmp);
	string_list_free(list);
	string_list_free(dir_list);
#endif

	/* add the path to the playlist */
	if (path != NULL) {
		char *path_utf8 = locale_to_utf8(path);
		mpdclient_cmd_add_path(c, path_utf8);
		g_free(path_utf8);
	}

	g_free(path);
	return 0;
}

static void
play_init(WINDOW *w, int cols, int rows)
{
	lw = list_window_init(w, cols, rows);
}

static gboolean
timer_hide_cursor(gpointer data)
{
	struct mpdclient *c = data;

	assert(options.hide_cursor > 0);
	assert(timer_hide_cursor_id != 0);

	timer_hide_cursor_id = 0;

	/* hide the cursor when mpd is playing and the user is inactive */

	if (c->status != NULL && c->status->state == MPD_STATUS_STATE_PLAY) {
		lw->hide_cursor = true;
		playlist_repaint();
	} else
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);

	return FALSE;
}

static void
play_open(mpdclient_t *c)
{
	static gboolean install_cb = TRUE;

	playlist = &c->playlist;

	assert(timer_hide_cursor_id == 0);
	if (options.hide_cursor > 0) {
		lw->hide_cursor = false;
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);
	}

	if (install_cb) {
		mpdclient_install_playlist_callback(c, playlist_changed_callback);
		install_cb = FALSE;
	}
}

static void
play_close(void)
{
	if (timer_hide_cursor_id != 0) {
		g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = 0;
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
play_paint(void)
{
	list_window_paint(lw, list_callback, NULL);
}

static void
play_update(mpdclient_t *c)
{
	static int prev_song_id = -1;

	current_song_id = c->song != NULL && c->status != NULL &&
		!IS_STOPPED(c->status->state) ? c->song->id : -1;

	if (current_song_id != prev_song_id) {
		prev_song_id = current_song_id;

		/* center the cursor */
		if (options.auto_center && current_song_id != -1)
			center_playing_item(c);

		playlist_repaint();
#ifndef NCMPC_MINI
	} else if (options.scroll) {
		/* always repaint if horizontal scrolling is
		   enabled */
		playlist_repaint();
#endif
	}
}

#ifdef HAVE_GETMOUSE
static bool
handle_mouse_event(struct mpdclient *c)
{
	int row;
	unsigned selected;
	unsigned long bstate;

	if (screen_get_mouse_event(c, &bstate, &row) ||
	    list_window_mouse(lw, playlist_length(playlist), bstate, row)) {
		playlist_repaint();
		return true;
	}

	if (bstate & BUTTON1_DOUBLE_CLICKED) {
		/* stop */
		screen_cmd(c, CMD_STOP);
		return true;
	}

	selected = lw->start + row;

	if (bstate & BUTTON1_CLICKED) {
		/* play */
		if (lw->start + row < playlist_length(playlist))
			mpdclient_cmd_play(c, lw->start + row);
	} else if (bstate & BUTTON3_CLICKED) {
		/* delete */
		if (selected == lw->selected)
			mpdclient_cmd_delete(c, lw->selected);
	}

	lw->selected = selected;
	list_window_check_selected(lw, playlist_length(playlist));
	playlist_repaint();

	return true;
}
#endif

static bool
play_cmd(mpdclient_t *c, command_t cmd)
{
	lw->hide_cursor = false;

	if (options.hide_cursor > 0) {
		if (timer_hide_cursor_id != 0)
			g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);
	}

	if (list_window_cmd(lw, playlist_length(&c->playlist), cmd)) {
		playlist_repaint();
		return true;
	}

	switch(cmd) {
	case CMD_PLAY:
		mpdclient_cmd_play(c, lw->selected);
		return true;
	case CMD_DELETE:
		mpdclient_cmd_delete(c, lw->selected);
		return true;
	case CMD_SAVE_PLAYLIST:
		playlist_save(c, NULL, NULL);
		return true;
	case CMD_ADD:
		handle_add_to_playlist(c);
		return true;
	case CMD_SCREEN_UPDATE:
		center_playing_item(c);
		playlist_repaint();
		return false;

	case CMD_LIST_MOVE_UP:
		mpdclient_cmd_move(c, lw->selected, lw->selected-1);
		return true;
	case CMD_LIST_MOVE_DOWN:
		mpdclient_cmd_move(c, lw->selected, lw->selected+1);
		return true;
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(lw, playlist_length(&c->playlist),
			    cmd, list_callback, NULL);
		playlist_repaint();
		return true;

#ifdef HAVE_GETMOUSE
	case CMD_MOUSE_EVENT:
		return handle_mouse_event(c);
#endif

#ifdef ENABLE_SONG_SCREEN
	case CMD_VIEW:
		if (lw->selected < playlist_length(&c->playlist)) {
			screen_song_switch(c, playlist_get(&c->playlist, lw->selected));
			return true;
		}

		break;
#endif

	case CMD_LOCATE:
		if (lw->selected < playlist_length(&c->playlist)) {
			screen_file_goto_song(c, playlist_get(&c->playlist, lw->selected));
			return true;
		}

		break;

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		if (lw->selected < playlist_length(&c->playlist)) {
			screen_lyrics_switch(c, playlist_get(&c->playlist, lw->selected));
			return true;
		}

		break;
#endif

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_playlist = {
	.init = play_init,
	.exit = play_exit,
	.open = play_open,
	.close = play_close,
	.resize = play_resize,
	.paint = play_paint,
	.update = play_update,
	.cmd = play_cmd,
	.get_title = play_title,
};
