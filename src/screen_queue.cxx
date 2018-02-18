/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "screen_queue.hxx"
#include "screen_interface.hxx"
#include "screen_file.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "save_playlist.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "options.hxx"
#include "mpdclient.hxx"
#include "utils.hxx"
#include "strfsong.hxx"
#include "wreadln.hxx"
#include "song_paint.hxx"
#include "screen.hxx"
#include "screen_utils.hxx"
#include "screen_song.hxx"
#include "screen_lyrics.hxx"
#include "db_completion.hxx"
#include "Compiler.h"

#ifndef NCMPC_MINI
#include "hscroll.hxx"
#endif

#include <mpd/client.h>

#include <ctype.h>
#include <string.h>
#include <glib.h>

#define MAX_SONG_LENGTH 512

#ifndef NCMPC_MINI
static struct hscroll hscroll;
#endif

static struct mpdclient_playlist *playlist;
static int current_song_id = -1;
static int selected_song_id = -1;
static ListWindow *lw;
static guint timer_hide_cursor_id;

static struct queue_screen_data {
	unsigned last_connection_id;
	char *connection_name;
} queue_screen;

static void
screen_queue_paint();

static void
screen_queue_repaint()
{
	screen_queue_paint();
	wrefresh(lw->w);
}

static const struct mpd_song *
screen_queue_selected_song()
{
	return !lw->range_selection &&
		lw->selected < playlist_length(playlist)
		? playlist_get(playlist, lw->selected)
		: nullptr;
}

static void
screen_queue_save_selection()
{
	selected_song_id = screen_queue_selected_song() != nullptr
		? (int)mpd_song_get_id(screen_queue_selected_song())
		: -1;
}

static void
screen_queue_restore_selection()
{
	list_window_set_length(lw, playlist_length(playlist));

	if (selected_song_id < 0)
		/* there was no selection */
		return;

	const struct mpd_song *song = screen_queue_selected_song();
	if (song != nullptr &&
	    mpd_song_get_id(song) == (unsigned)selected_song_id)
		/* selection is still valid */
		return;

	int pos = playlist_get_index_from_id(playlist, selected_song_id);
	if (pos >= 0)
		list_window_set_cursor(lw, pos);

	screen_queue_save_selection();
}

static const char *
screen_queue_lw_callback(unsigned idx, gcc_unused void *data)
{
	static char songname[MAX_SONG_LENGTH];

	assert(playlist != nullptr);
	assert(idx < playlist_length(playlist));

	struct mpd_song *song = playlist_get(playlist, idx);

	strfsong(songname, MAX_SONG_LENGTH, options.list_format, song);

	return songname;
}

static void
center_playing_item(const struct mpd_status *status, bool center_cursor)
{
	if (status == nullptr ||
	    (mpd_status_get_state(status) != MPD_STATE_PLAY &&
	     mpd_status_get_state(status) != MPD_STATE_PAUSE))
		return;

	/* try to center the song that are playing */
	int idx = mpd_status_get_song_pos(status);
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

gcc_pure
static int
get_current_song_id(const struct mpd_status *status)
{
	return status != nullptr &&
		(mpd_status_get_state(status) == MPD_STATE_PLAY ||
		 mpd_status_get_state(status) == MPD_STATE_PAUSE)
		? (int)mpd_status_get_song_id(status)
		: -1;
}

static bool
screen_queue_song_change(const struct mpd_status *status)
{
	if (get_current_song_id(status) == current_song_id)
		return false;

	current_song_id = get_current_song_id(status);

	/* center the cursor */
	if (options.auto_center && !lw->range_selection)
		center_playing_item(status, false);

	return true;
}

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

struct completion_callback_data {
	GList **list;
	GList **dir_list;
	struct mpdclient *c;
};

static void add_pre_completion_cb(GCompletion *gcmp, gchar *line, void *data)
{
	auto *tmp = (struct completion_callback_data *)data;
	GList **dir_list = tmp->dir_list;
	GList **list = tmp->list;
	struct mpdclient *c = tmp->c;

	if (*list == nullptr) {
		/* create initial list */
		*list = gcmp_list_from_path(c, "", nullptr, GCMP_TYPE_RFILE);
		g_completion_add_items(gcmp, *list);
	} else if (line && line[0] && line[strlen(line)-1]=='/' &&
		   string_list_find(*dir_list, line) == nullptr) {
		/* add directory content to list */
		add_dir(gcmp, line, dir_list, list, c);
	}
}

static void add_post_completion_cb(GCompletion *gcmp, gchar *line,
				   GList *items, void *data)
{
	auto *tmp = (struct completion_callback_data *)data;
	GList **dir_list = tmp->dir_list;
	GList **list = tmp->list;
	struct mpdclient *c = tmp->c;

	if (g_list_length(items) >= 1)
		screen_display_completion_list(items);

	if (line && line[0] && line[strlen(line) - 1] == '/' &&
	    string_list_find(*dir_list, line) == nullptr) {
		/* add directory content to list */
		add_dir(gcmp, line, dir_list, list, c);
	}
}
#endif

static int
handle_add_to_playlist(struct mpdclient *c)
{
#ifndef NCMPC_MINI
	/* initialize completion support */
	GCompletion *gcmp = g_completion_new(nullptr);
	g_completion_set_compare(gcmp, completion_strncmp);

	GList *list = nullptr;
	GList *dir_list = nullptr;
	struct completion_callback_data data = {
		.list = &list,
		.dir_list = &dir_list,
		.c = c,
	};

	wrln_completion_callback_data = &data;
	wrln_pre_completion_callback = add_pre_completion_cb;
	wrln_post_completion_callback = add_post_completion_cb;
#else
	GCompletion *gcmp = nullptr;
#endif

	/* get path */
	char *path = screen_readln(_("Add"),
				   nullptr,
				   nullptr,
				   gcmp);

	/* destroy completion data */
#ifndef NCMPC_MINI
	wrln_completion_callback_data = nullptr;
	wrln_pre_completion_callback = nullptr;
	wrln_post_completion_callback = nullptr;
	g_completion_free(gcmp);
	string_list_free(list);
	string_list_free(dir_list);
#endif

	/* add the path to the playlist */
	if (path != nullptr) {
		char *path_utf8 = locale_to_utf8(path);
		mpdclient_cmd_add_path(c, path_utf8);
		g_free(path_utf8);
	}

	g_free(path);
	return 0;
}

static void
screen_queue_init(WINDOW *w, unsigned cols, unsigned rows)
{
	lw = new ListWindow(w, cols, rows);

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_init(&hscroll, w, options.scroll_sep);
#endif
}

static gboolean
timer_hide_cursor(gpointer data)
{
	auto *c = (struct mpdclient *)data;

	assert(options.hide_cursor > 0);
	assert(timer_hide_cursor_id != 0);

	timer_hide_cursor_id = 0;

	/* hide the cursor when mpd is playing and the user is inactive */

	if (c->status != nullptr &&
	    mpd_status_get_state(c->status) == MPD_STATE_PLAY) {
		lw->hide_cursor = true;
		screen_queue_repaint();
	} else
		timer_hide_cursor_id = g_timeout_add_seconds(options.hide_cursor,
							     timer_hide_cursor, c);

	return false;
}

static void
screen_queue_open(struct mpdclient *c)
{
	playlist = &c->playlist;

	assert(timer_hide_cursor_id == 0);
	if (options.hide_cursor > 0) {
		lw->hide_cursor = false;
		timer_hide_cursor_id = g_timeout_add_seconds(options.hide_cursor,
							     timer_hide_cursor, c);
	}

	screen_queue_restore_selection();
	screen_queue_song_change(c->status);
}

static void
screen_queue_close()
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
screen_queue_resize(unsigned cols, unsigned rows)
{
	list_window_resize(lw, cols, rows);
}


static void
screen_queue_exit()
{
	delete lw;
	g_free(queue_screen.connection_name);
}

static const char *
screen_queue_title(char *str, size_t size)
{
       if (queue_screen.connection_name == nullptr)
	       return _("Queue");

       g_snprintf(str, size, _("Queue on %s"), queue_screen.connection_name);
	return str;
}

static void
screen_queue_paint_callback(WINDOW *w, unsigned i,
			    unsigned y, unsigned width,
			    bool selected, gcc_unused const void *data)
{
	assert(playlist != nullptr);
	assert(i < playlist_length(playlist));

	const struct mpd_song *song = playlist_get(playlist, i);

	struct hscroll *row_hscroll = nullptr;
#ifndef NCMPC_MINI
	row_hscroll = selected && options.scroll && lw->selected == i
		? &hscroll : nullptr;
#endif

	paint_song_row(w, y, width, selected,
		       (int)mpd_song_get_id(song) == current_song_id,
		       song, row_hscroll, options.list_format);
}

static void
screen_queue_paint()
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll_clear(&hscroll);
#endif

	list_window_paint2(lw, screen_queue_paint_callback, nullptr);
}

static void
screen_queue_update(struct mpdclient *c)
{
	if (c->connection_id != queue_screen.last_connection_id) {
		queue_screen.last_connection_id = c->connection_id;
		g_free(queue_screen.connection_name);
		queue_screen.connection_name = mpdclient_settings_name(c);
	}

	if (c->events & MPD_IDLE_QUEUE)
		screen_queue_restore_selection();
	else
		/* the queue size may have changed, even if we havn't
		   received the QUEUE idle event yet */
		list_window_set_length(lw, playlist_length(playlist));

	if (((c->events & MPD_IDLE_PLAYER) != 0 &&
	     screen_queue_song_change(c->status)) ||
	    c->events & MPD_IDLE_QUEUE)
		/* the queue or the current song has changed, we must
		   paint the new version */
		screen_queue_paint();
}

#ifdef HAVE_GETMOUSE
static bool
screen_queue_mouse(struct mpdclient *c, gcc_unused int x, int row,
		   mmask_t bstate)
{
	if (list_window_mouse(lw, bstate, row)) {
		screen_queue_paint();
		return true;
	}

	if (bstate & BUTTON1_DOUBLE_CLICKED) {
		/* stop */
		screen.OnCommand(c, CMD_STOP);
		return true;
	}

	const unsigned old_selected = lw->selected;
	list_window_set_cursor(lw, lw->start + row);

	if (bstate & BUTTON1_CLICKED) {
		/* play */
		const struct mpd_song *song = screen_queue_selected_song();
		if (song != nullptr) {
			struct mpd_connection *connection =
				mpdclient_get_connection(c);

			if (connection != nullptr &&
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
	screen_queue_paint();

	return true;
}
#endif

static bool
screen_queue_cmd(struct mpdclient *c, command_t cmd)
{
	struct mpd_connection *connection;
	static command_t cached_cmd = CMD_NONE;

	const command_t prev_cmd = cached_cmd;
	cached_cmd = cmd;

	lw->hide_cursor = false;

	if (options.hide_cursor > 0) {
		if (timer_hide_cursor_id != 0)
			g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = g_timeout_add_seconds(options.hide_cursor,
							     timer_hide_cursor, c);
	}

	if (list_window_cmd(lw, cmd)) {
		screen_queue_save_selection();
		screen_queue_paint();
		return true;
	}

	switch(cmd) {
	case CMD_SCREEN_UPDATE:
		center_playing_item(c->status, prev_cmd == CMD_SCREEN_UPDATE);
		screen_queue_paint();
		return false;
	case CMD_SELECT_PLAYING:
		list_window_set_cursor(lw, playlist_get_index(&c->playlist,
							      c->song));
		screen_queue_save_selection();
		screen_queue_paint();
		return true;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(lw, cmd, screen_queue_lw_callback, nullptr);
		screen_queue_save_selection();
		screen_queue_paint();
		return true;
	case CMD_LIST_JUMP:
		screen_jump(lw, screen_queue_lw_callback, nullptr, nullptr, nullptr);
		screen_queue_save_selection();
		screen_queue_paint();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		if (screen_queue_selected_song() != nullptr) {
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
		if (playlist_length(&c->playlist) > 0)
			screen.Swap(c, playlist_get(&c->playlist, lw->selected));
		else
			screen.Swap(c, nullptr);
		return true;

	default:
		break;
	}

	if (!mpdclient_is_connected(c))
		return false;

	switch(cmd) {
		const struct mpd_song *song;
		ListWindowRange range;

	case CMD_PLAY:
		song = screen_queue_selected_song();
		if (song == nullptr)
			return false;

		connection = mpdclient_get_connection(c);
		if (connection != nullptr &&
		    !mpd_run_play_id(connection, mpd_song_get_id(song)))
			mpdclient_handle_error(c);

		return true;

	case CMD_DELETE:
		list_window_get_range(lw, &range);
		mpdclient_cmd_delete_range(c, range.start, range.end);

		list_window_set_cursor(lw, range.start);
		return true;

	case CMD_SAVE_PLAYLIST:
		playlist_save(c, nullptr, nullptr);
		return true;

	case CMD_ADD:
		handle_add_to_playlist(c);
		return true;

	case CMD_SHUFFLE:
		list_window_get_range(lw, &range);
		if (range.end <= range.start + 1)
			/* No range selection, shuffle all list. */
			break;

		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			return true;

		if (mpd_run_shuffle_range(connection, range.start, range.end))
			screen_status_message(_("Shuffled queue"));
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

		if (lw->range_selection)
			list_window_scroll_to(lw, lw->range_base);
		list_window_scroll_to(lw, lw->selected);

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

		if (lw->range_selection)
			list_window_scroll_to(lw, lw->range_base);
		list_window_scroll_to(lw, lw->selected);

		screen_queue_save_selection();
		return true;

	case CMD_LOCATE:
		if (screen_queue_selected_song() != nullptr) {
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
#ifdef HAVE_GETMOUSE
	.mouse = screen_queue_mouse,
#endif
	.get_title = screen_queue_title,
};
