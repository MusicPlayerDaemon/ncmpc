/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "screen_queue.h"
#include "screen_interface.h"
#include "screen_file.h"
#include "screen_message.h"
#include "screen_find.h"
#include "config.h"
#include "i18n.h"
#include "charset.h"
#include "options.h"
#include "mpdclient.h"
#include "utils.h"
#include "strfsong.h"
#include "wreadln.h"
#include "song_paint.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_song.h"
#include "screen_lyrics.h"

#ifndef NCMPC_MINI
#include "hscroll.h"
#endif

#include <mpd/client.h>

#include <ctype.h>
#include <string.h>
#include <glib.h>

#define MAX_SONG_LENGTH 512

#ifndef NCMPC_MINI
typedef struct
{
	GList **list;
	GList **dir_list;
	struct mpdclient *c;
} completion_callback_data_t;

static struct hscroll hscroll;
#endif

static struct mpdclient_playlist *playlist;
static int current_song_id = -1;
static int selected_song_id = -1;
static struct list_window *lw;
static guint timer_hide_cursor_id;

static void
screen_queue_paint(void);

static void
screen_queue_repaint(void)
{
	screen_queue_paint();
	wrefresh(lw->w);
}

static const struct mpd_song *
screen_queue_selected_song(void)
{
	return !lw->range_selection &&
		lw->selected < playlist_length(playlist)
		? playlist_get(playlist, lw->selected)
		: NULL;
}

static void
screen_queue_save_selection(void)
{
	selected_song_id = screen_queue_selected_song() != NULL
		? (int)mpd_song_get_id(screen_queue_selected_song())
		: -1;
}

static void
screen_queue_restore_selection(void)
{
	const struct mpd_song *song;
	int pos;

	list_window_set_length(lw, playlist_length(playlist));

	if (selected_song_id < 0)
		/* there was no selection */
		return;

	song = screen_queue_selected_song();
	if (song != NULL &&
	    mpd_song_get_id(song) == (unsigned)selected_song_id)
		/* selection is still valid */
		return;

	pos = playlist_get_index_from_id(playlist, selected_song_id);
	if (pos >= 0)
		list_window_set_cursor(lw, pos);

	screen_queue_save_selection();
}

static const char *
screen_queue_lw_callback(unsigned idx, G_GNUC_UNUSED void *data)
{
	static char songname[MAX_SONG_LENGTH];
	struct mpd_song *song;

	assert(playlist != NULL);
	assert(idx < playlist_length(playlist));

	song = playlist_get(playlist, idx);

	strfsong(songname, MAX_SONG_LENGTH, options.list_format, song);

	return songname;
}

static void
center_playing_item(struct mpdclient *c, bool center_cursor)
{
	int idx;

	/* try to center the song that are playing */
	idx = playlist_get_index(&c->playlist, c->song);
	if (idx < 0)
		return;

	list_window_center(lw, idx);

	if (center_cursor) {
		list_window_set_cursor(lw, idx);
		return;
	}

	/* make sure the cursor is in the window */
	list_window_fetch_cursor(lw);
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
	struct mpd_connection *connection;
	gchar *filename, *filename_utf8;
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
		filename = screen_readln(_("Save playlist as"),
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

	connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return -1;

	filename_utf8 = locale_to_utf8(filename);
	if (!mpd_run_save(connection, filename_utf8)) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_EXIST &&
		    mpd_connection_clear_error(connection)) {
			char *buf;
			int key;

			buf = g_strdup_printf(_("Replace %s [%s/%s] ? "),
					      filename, YES, NO);
			key = tolower(screen_getch(buf));
			g_free(buf);

			if (key != YES[0]) {
				g_free(filename_utf8);
				g_free(filename);
				screen_status_printf(_("Aborted"));
				return -1;
			}

			if (!mpd_run_rm(connection, filename_utf8) ||
			    !mpd_run_save(connection, filename_utf8)) {
				mpdclient_handle_error(c);
				g_free(filename_utf8);
				g_free(filename);
				return -1;
			}
		} else {
			mpdclient_handle_error(c);
			g_free(filename_utf8);
			g_free(filename);
			return -1;
		}
	}

	c->events |= MPD_IDLE_STORED_PLAYLIST;

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
	path = screen_readln(_("Add"),
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
screen_queue_init(WINDOW *w, int cols, int rows)
{
	lw = list_window_init(w, cols, rows);

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_init(&hscroll, w, options.scroll_sep);
#endif
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
		screen_queue_repaint();
	} else
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);

	return FALSE;
}

static void
screen_queue_open(struct mpdclient *c)
{
	playlist = &c->playlist;

	assert(timer_hide_cursor_id == 0);
	if (options.hide_cursor > 0) {
		lw->hide_cursor = false;
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);
	}

	screen_queue_restore_selection();
}

static void
screen_queue_close(void)
{
	if (timer_hide_cursor_id != 0) {
		g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = 0;
	}

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&hscroll);
#endif
}

static void
screen_queue_resize(int cols, int rows)
{
	list_window_resize(lw, cols, rows);
}


static void
screen_queue_exit(void)
{
	list_window_free(lw);
}

static const char *
screen_queue_title(char *str, size_t size)
{
	if (options.host == NULL)
		return _("Playlist");

	g_snprintf(str, size, _("Playlist on %s"), options.host);
	return str;
}

static void
screen_queue_paint_callback(WINDOW *w, unsigned i,
			    unsigned y, unsigned width,
			    bool selected, G_GNUC_UNUSED void *data)
{
	const struct mpd_song *song;
	struct hscroll *row_hscroll;

	assert(playlist != NULL);
	assert(i < playlist_length(playlist));

	song = playlist_get(playlist, i);

#ifdef NCMPC_MINI
	row_hscroll = NULL;
#else
	row_hscroll = selected && options.scroll && lw->selected == i
		? &hscroll : NULL;
#endif

	paint_song_row(w, y, width, selected,
		       (int)mpd_song_get_id(song) == current_song_id,
		       song, row_hscroll);
}

static void
screen_queue_paint(void)
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&hscroll);
#endif

	list_window_paint2(lw, screen_queue_paint_callback, NULL);
}

G_GNUC_PURE
static int
get_current_song_id(const struct mpd_status *status)
{
	return status != NULL &&
		(mpd_status_get_state(status) == MPD_STATE_PLAY ||
		 mpd_status_get_state(status) == MPD_STATE_PAUSE)
		? (int)mpd_status_get_song_id(status)
		: -1;
}

static void
screen_queue_update(struct mpdclient *c)
{
	if (c->events & MPD_IDLE_PLAYLIST)
		screen_queue_restore_selection();

	if ((c->events & MPD_IDLE_PLAYER) != 0 &&
	    get_current_song_id(c->status) != current_song_id) {
		current_song_id = get_current_song_id(c->status);

		/* center the cursor */
		if (options.auto_center && current_song_id != -1 && ! lw->range_selection)
			center_playing_item(c, false);

		screen_queue_repaint();
	} else if (c->events & MPD_IDLE_PLAYLIST) {
		/* the playlist has changed, we must paint the new
		   version */
		screen_queue_repaint();
	}
}

#ifdef HAVE_GETMOUSE
static bool
handle_mouse_event(struct mpdclient *c)
{
	int row;
	unsigned long bstate;
	unsigned old_selected;

	if (screen_get_mouse_event(c, &bstate, &row) ||
	    list_window_mouse(lw, bstate, row)) {
		screen_queue_repaint();
		return true;
	}

	if (bstate & BUTTON1_DOUBLE_CLICKED) {
		/* stop */
		screen_cmd(c, CMD_STOP);
		return true;
	}

	old_selected = lw->selected;
	list_window_set_cursor(lw, lw->start + row);

	if (bstate & BUTTON1_CLICKED) {
		/* play */
		const struct mpd_song *song = screen_queue_selected_song();
		if (song != NULL) {
			struct mpd_connection *connection =
				mpdclient_get_connection(c);

			if (connection != NULL &&
			    !mpd_run_play_id(connection,
					     mpd_song_get_id(song)))
				mpdclient_handle_error(c);
		}
	} else if (bstate & BUTTON3_CLICKED) {
		/* delete */
		if (lw->selected == old_selected)
			mpdclient_cmd_delete(c, lw->selected);

		list_window_set_length(lw, playlist_length(playlist));
	}

	screen_queue_save_selection();
	screen_queue_repaint();

	return true;
}
#endif

static bool
screen_queue_cmd(struct mpdclient *c, command_t cmd)
{
	struct mpd_connection *connection;
	static command_t cached_cmd = CMD_NONE;
	command_t prev_cmd = cached_cmd;
	struct list_window_range range;
	const struct mpd_song *song;

	cached_cmd = cmd;

	lw->hide_cursor = false;

	if (options.hide_cursor > 0) {
		if (timer_hide_cursor_id != 0)
			g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = g_timeout_add(options.hide_cursor * 1000,
						     timer_hide_cursor, c);
	}

	if (list_window_cmd(lw, cmd)) {
		screen_queue_save_selection();
		screen_queue_repaint();
		return true;
	}

	switch(cmd) {
	case CMD_SCREEN_UPDATE:
		center_playing_item(c, prev_cmd == CMD_SCREEN_UPDATE);
		screen_queue_repaint();
		return false;
	case CMD_SELECT_PLAYING:
		list_window_set_cursor(lw, playlist_get_index(&c->playlist,
							      c->song));
		screen_queue_save_selection();
		screen_queue_repaint();
		return true;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(lw, cmd, screen_queue_lw_callback, NULL);
		screen_queue_save_selection();
		screen_queue_repaint();
		return true;
	case CMD_LIST_JUMP:
		screen_jump(lw, screen_queue_lw_callback, NULL, NULL);
		screen_queue_save_selection();
		screen_queue_repaint();
		return true;

#ifdef HAVE_GETMOUSE
	case CMD_MOUSE_EVENT:
		return handle_mouse_event(c);
#endif

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		if (screen_queue_selected_song() != NULL) {
			screen_song_switch(c, screen_queue_selected_song());
			return true;
		}

		break;
#endif

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

	if (!mpdclient_is_connected(c))
		return false;

	switch(cmd) {
	case CMD_PLAY:
		song = screen_queue_selected_song();
		if (song == NULL)
			return false;

		connection = mpdclient_get_connection(c);
		if (connection != NULL &&
		    !mpd_run_play_id(connection, mpd_song_get_id(song)))
			mpdclient_handle_error(c);

		return true;

	case CMD_DELETE:
		list_window_get_range(lw, &range);
		mpdclient_cmd_delete_range(c, range.start, range.end);

		list_window_set_cursor(lw, range.start);
		return true;

	case CMD_SAVE_PLAYLIST:
		playlist_save(c, NULL, NULL);
		return true;

	case CMD_ADD:
		handle_add_to_playlist(c);
		return true;

	case CMD_SHUFFLE:
		list_window_get_range(lw, &range);
		if (range.end < range.start + 1)
			/* No range selection, shuffle all list. */
			break;

		connection = mpdclient_get_connection(c);
		if (connection == NULL)
			return true;

		if (mpd_run_shuffle_range(connection, range.start, range.end))
			screen_status_message(_("Shuffled playlist"));
		else
			mpdclient_handle_error(c);
		return true;

	case CMD_LIST_MOVE_UP:
		list_window_get_range(lw, &range);
		if (range.start == 0 || range.end <= range.start)
			return false;

		if (!mpdclient_cmd_move(c, range.end - 1, range.start - 1))
			return true;

		lw->selected--;
		lw->range_base--;

		screen_queue_save_selection();
		return true;

	case CMD_LIST_MOVE_DOWN:
		list_window_get_range(lw, &range);
		if (range.end >= playlist_length(&c->playlist))
			return false;

		if (!mpdclient_cmd_move(c, range.start, range.end))
			return true;

		lw->selected++;
		lw->range_base++;

		screen_queue_save_selection();
		return true;

	case CMD_LOCATE:
		if (screen_queue_selected_song() != NULL) {
			screen_file_goto_song(c, screen_queue_selected_song());
			return true;
		}

		break;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_queue = {
	.init = screen_queue_init,
	.exit = screen_queue_exit,
	.open = screen_queue_open,
	.close = screen_queue_close,
	.resize = screen_queue_resize,
	.paint = screen_queue_paint,
	.update = screen_queue_update,
	.cmd = screen_queue_cmd,
	.get_title = screen_queue_title,
};
