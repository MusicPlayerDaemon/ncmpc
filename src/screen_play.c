/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#include <mpd/client.h>

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
	struct mpdclient *c;
} completion_callback_data_t;

static bool must_scroll;
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
playlist_changed_callback(struct mpdclient *c, int event, gpointer data)
{
	switch (event) {
	case PLAYLIST_EVENT_DELETE:
		break;
	case PLAYLIST_EVENT_MOVE:
		if(lw->range_selection < 0)
		{
			lw->selected = *((int *) data);
			if (lw->selected < lw->start)
				lw->start--;
		}
		break;
	default:
		break;
	}

	list_window_check_selected(lw, c->playlist.list->len);
	playlist_repaint_if_active();
}

#ifndef NCMPC_MINI
static char *
format_duration(unsigned duration)
{
	if (duration == 0)
		return NULL;

	return g_strdup_printf("%d:%02d", duration / 60, duration % 60);
}
#endif

static const char *
list_callback(unsigned idx, bool *highlight, char **second_column, G_GNUC_UNUSED void *data)
{
	static char songname[MAX_SONG_LENGTH];
#ifndef NCMPC_MINI
	static scroll_state_t st;
#endif
	struct mpd_song *song;

	if (playlist == NULL || idx >= playlist_length(playlist))
		return NULL;

	song = playlist_get(playlist, idx);
	if ((int)mpd_song_get_id(song) == current_song_id)
		*highlight = true;

	strfsong(songname, MAX_SONG_LENGTH, options.list_format, song);

#ifndef NCMPC_MINI
	if(second_column)
		*second_column = format_duration(mpd_song_get_duration(song));

	if (idx == lw->selected)
	{
		int second_column_len = 0;
		if (second_column != NULL && *second_column != NULL)
			second_column_len = strlen(*second_column);
		if (options.scroll && utf8_width(songname) > (unsigned)(COLS - second_column_len - 1) )
		{
			static unsigned current_song;
			char *tmp;

			must_scroll = true;

			if (current_song != lw->selected) {
				st.offset = 0;
				current_song = lw->selected;
			}

			tmp = strscroll(songname, options.scroll_sep,
					MAX_SONG_LENGTH, &st);
			g_strlcpy(songname, tmp, MAX_SONG_LENGTH);
			g_free(tmp);
		}
		else
			st.offset = 0;
	}
#else
	(void)second_column;
#endif

	return songname;
}

static void
center_playing_item(struct mpdclient *c, bool center_cursor)
{
	unsigned length = c->playlist.list->len;
	int idx;

	if (!c->song || c->status == NULL ||
	    IS_STOPPED(mpd_status_get_state(c->status)))
		return;

	/* try to center the song that are playing */
	idx = playlist_get_index(&c->playlist, c->song);
	if (idx < 0)
		return;

	if (length < lw->rows)
	{
		if (center_cursor)
			list_window_set_selected(lw, idx);
		return;
	}

	list_window_center(lw, length, idx);

	if (center_cursor) {
		list_window_set_selected(lw, idx);
		return;
	}

	/* make sure the cursor is in the window */
	if (lw->selected < lw->start + options.scroll_offset) {
		if (lw->start > 0)
			lw->selected = lw->start + options.scroll_offset;
		if (lw->range_selection) {
			lw->selected_start = lw->range_base;
			lw->selected_end = lw->selected;
		} else {
			lw->selected_start = lw->selected;
			lw->selected_end = lw->selected;
		}
	} else if (lw->selected > lw->start + lw->rows - 1 - options.scroll_offset) {
		if (lw->start + lw->rows < length)
			lw->selected = lw->start + lw->rows - 1 - options.scroll_offset;
		if (lw->range_selection) {
			lw->selected_start = lw->selected;
			lw->selected_end = lw->range_base;
		} else {
			lw->selected_start = lw->selected;
			lw->selected_end = lw->selected;
		}
	}
}

#ifndef NCMPC_MINI
static void
save_pre_completion_cb(GCompletion *gcmp, G_GNUC_UNUSED gchar *line,
		       void *data)
{
	completion_callback_data_t *tmp = (completion_callback_data_t *)data;
	GList **list = tmp->list;
	struct mpdclient *c = tmp->c;

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
playlist_save(struct mpdclient *c, char *name, char *defaultname)
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
					 _("Save playlist as"),
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

	if (error) {
		gint code = GET_ACK_ERROR_CODE(error);

		if (code == MPD_SERVER_ERROR_EXIST) {
			char *buf;
			int key;

			buf = g_strdup_printf(_("Replace %s [%s/%s] ? "),
					      filename, YES, NO);
			key = tolower(screen_getch(screen.status_window.w,
						   buf));
			g_free(buf);

			if (key != YES[0]) {
				g_free(filename_utf8);
				g_free(filename);
				screen_status_printf(_("Aborted"));
				return -1;
			}

			error = mpdclient_cmd_delete_playlist(c, filename_utf8);
			if (error) {
				g_free(filename_utf8);
				g_free(filename);
				return -1;
			}

			error = mpdclient_cmd_save_playlist(c, filename_utf8);
			if (error) {
				g_free(filename_utf8);
				g_free(filename);
				return error;
			}
		} else {
			g_free(filename_utf8);
			g_free(filename);
			return -1;
		}
	}

	g_free(filename_utf8);

	/* success */
	screen_status_printf(_("Saved %s"), filename);
	g_free(filename);
	return 0;
}

#ifndef NCMPC_MINI
static void add_dir(GCompletion *gcmp, gchar *dir, GList **dir_list,
		    GList **list, struct mpdclient *c)
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
	struct mpdclient *c = tmp->c;

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
	struct mpdclient *c = tmp->c;

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
handle_add_to_playlist(struct mpdclient *c)
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
			     _("Add"),
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

	if (c->status != NULL &&
	    mpd_status_get_state(c->status) == MPD_STATE_PLAY) {
		lw->hide_cursor = true;
		playlist_repaint();
	} else
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);

	return FALSE;
}

static void
play_open(struct mpdclient *c)
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
	if (options.host == NULL)
		return _("Playlist");

	g_snprintf(str, size, _("Playlist on %s"), options.host);
	return str;
}

static void
play_paint(void)
{
#ifndef NCMPC_MINI
	must_scroll = false;
#endif

	list_window_paint(lw, list_callback, NULL);
}

static void
play_update(struct mpdclient *c)
{
	static int prev_song_id = -1;

	current_song_id = c->song != NULL && c->status != NULL &&
		!IS_STOPPED(mpd_status_get_state(c->status))
		? (int)mpd_song_get_id(c->song) : -1;

	if (current_song_id != prev_song_id) {
		prev_song_id = current_song_id;

		/* center the cursor */
		if (options.auto_center && current_song_id != -1 && ! lw->range_selection)
			center_playing_item(c, false);

		playlist_repaint();
#ifndef NCMPC_MINI
	} else if (options.scroll && must_scroll) {
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
play_cmd(struct mpdclient *c, command_t cmd)
{
	static command_t cached_cmd = CMD_NONE;
	command_t prev_cmd = cached_cmd;
	cached_cmd = cmd;

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
	{
		int i = lw->selected_end, start = lw->selected_start;
		for(; i >= start; --i)
			mpdclient_cmd_delete(c, i);

		i++;
		if(i >= (int)playlist_length(&c->playlist))
			i--;
		lw->selected = i;
		lw->selected_start = i;
		lw->selected_end = i;
		lw->range_selection = false;

		return true;
	}
	case CMD_SAVE_PLAYLIST:
		playlist_save(c, NULL, NULL);
		return true;
	case CMD_ADD:
		handle_add_to_playlist(c);
		return true;
	case CMD_SCREEN_UPDATE:
		center_playing_item(c, prev_cmd == CMD_SCREEN_UPDATE);
		playlist_repaint();
		return false;
	case CMD_SELECT_PLAYING:
		list_window_set_selected(lw, playlist_get_index(&c->playlist,
								c->song));
		return true;
	case CMD_SHUFFLE:
	{
		if(!lw->range_selection)
			/* No range selection, shuffle all list. */
			break;

		if (mpdclient_cmd_shuffle_range(c, lw->selected_start, lw->selected_end+1) == 0)
			screen_status_message(_("Shuffled playlist"));

		return true;
	}
	case CMD_LIST_MOVE_UP:
		if(lw->selected_start == 0)
			return false;
		if(lw->range_selection)
		{
			unsigned i = lw->selected_start;
			unsigned last_selected = lw->selected;
			for(; i <= lw->selected_end; ++i)
				mpdclient_cmd_move(c, i, i-1);
			lw->selected_start--;
			lw->selected_end--;
			lw->selected = last_selected - 1;
			lw->range_base--;
		}
		else
		{
			mpdclient_cmd_move(c, lw->selected, lw->selected-1);
			lw->selected--;
			lw->selected_start--;
			lw->selected_end--;
		}
		return true;
	case CMD_LIST_MOVE_DOWN:
		if(lw->selected_end+1 >= playlist_length(&c->playlist))
			return false;
		if(lw->range_selection)
		{
			int i = lw->selected_end;
			unsigned last_selected = lw->selected;
			for(; i >= (int)lw->selected_start; --i)
				mpdclient_cmd_move(c, i, i+1);
			lw->selected_start++;
			lw->selected_end++;
			lw->selected = last_selected + 1;
			lw->range_base++;
		}
		else
		{
			mpdclient_cmd_move(c, lw->selected, lw->selected+1);
			lw->selected++;
			lw->selected_start++;
			lw->selected_end++;
		}
		return true;
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(lw, playlist_length(&c->playlist),
			    cmd, list_callback, NULL);
		playlist_repaint();
		return true;
	case CMD_LIST_JUMP:
		screen_jump(lw, list_callback, NULL);
		playlist_repaint();
		return true;

#ifdef HAVE_GETMOUSE
	case CMD_MOUSE_EVENT:
		return handle_mouse_event(c);
#endif

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
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
			struct mpd_song *selected = playlist_get(&c->playlist, lw->selected);
			bool follow = false;

			if (c->song && selected &&
			    !strcmp(mpd_song_get_uri(selected),
				    mpd_song_get_uri(c->song)))
				follow = true;

			screen_lyrics_switch(c, selected, follow);
			return true;
		}

		break;
#endif
	case CMD_SCREEN_SWAP:
		screen_swap(c, playlist_get(&c->playlist, lw->selected));
		return true;

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
