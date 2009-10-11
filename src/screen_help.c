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

#include "screen_help.h"
#include "screen_interface.h"
#include "screen_find.h"
#include "paint.h"
#include "charset.h"
#include "config.h"
#include "i18n.h"

#include <glib.h>

#include <assert.h>

struct help_text_row {
	signed char highlight;
	command_t command;
	const char *text;
};

static const struct help_text_row help_text[] = {
	{ 1, CMD_NONE, N_("Movement") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_LIST_PREVIOUS, NULL },
	{ 0, CMD_LIST_NEXT, NULL },
	{ 0, CMD_LIST_TOP, NULL },
	{ 0, CMD_LIST_MIDDLE, NULL },
	{ 0, CMD_LIST_BOTTOM, NULL },
	{ 0, CMD_LIST_PREVIOUS_PAGE, NULL },
	{ 0, CMD_LIST_NEXT_PAGE, NULL },
	{ 0, CMD_LIST_FIRST, NULL },
	{ 0, CMD_LIST_LAST, NULL },
	{ 0, CMD_LIST_RANGE_SELECT, NULL },
	{ 0, CMD_LIST_SCROLL_UP_LINE, NULL},
	{ 0, CMD_LIST_SCROLL_DOWN_LINE, NULL},
	{ 0, CMD_LIST_SCROLL_UP_HALF, NULL},
	{ 0, CMD_LIST_SCROLL_DOWN_HALF, NULL},
	{ 0, CMD_NONE, NULL },

	{ 0, CMD_SCREEN_PREVIOUS,NULL },
	{ 0, CMD_SCREEN_NEXT, NULL },
    { 0, CMD_SCREEN_SWAP, NULL },
	{ 0, CMD_SCREEN_HELP, NULL },
	{ 0, CMD_SCREEN_PLAY, NULL },
	{ 0, CMD_SCREEN_FILE, NULL },
#ifdef ENABLE_ARTIST_SCREEN
	{ 0, CMD_SCREEN_ARTIST, NULL },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ 0, CMD_SCREEN_SEARCH, NULL },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ 0, CMD_SCREEN_LYRICS, NULL },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ 0, CMD_SCREEN_OUTPUTS, NULL },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ 0, CMD_SCREEN_KEYDEF, NULL },
#endif

	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Global") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_STOP, NULL },
	{ 0, CMD_PAUSE, NULL },
	{ 0, CMD_CROP, NULL },
	{ 0, CMD_TRACK_NEXT, NULL },
	{ 0, CMD_TRACK_PREVIOUS, NULL },
	{ 0, CMD_SEEK_FORWARD, NULL },
	{ 0, CMD_SEEK_BACKWARD, NULL },
	{ 0, CMD_VOLUME_DOWN, NULL },
	{ 0, CMD_VOLUME_UP, NULL },
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_REPEAT, NULL },
	{ 0, CMD_RANDOM, NULL },
	{ 0, CMD_SINGLE, NULL },
	{ 0, CMD_CONSUME, NULL },
	{ 0, CMD_CROSSFADE, NULL },
	{ 0, CMD_SHUFFLE, NULL },
	{ 0, CMD_DB_UPDATE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_LIST_FIND, NULL },
	{ 0, CMD_LIST_RFIND, NULL },
	{ 0, CMD_LIST_FIND_NEXT, NULL },
	{ 0, CMD_LIST_RFIND_NEXT, NULL },
	{ 0, CMD_LIST_JUMP, NULL },
	{ 0, CMD_TOGGLE_FIND_WRAP, NULL },
	{ 0, CMD_LOCATE, NULL },
	{ 0, CMD_SCREEN_SONG, NULL },
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_QUIT, NULL },

	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Playlist screen") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_PLAY, N_("Play") },
	{ 0, CMD_DELETE, NULL },
	{ 0, CMD_CLEAR, NULL },
	{ 0, CMD_LIST_MOVE_UP, N_("Move song up") },
	{ 0, CMD_LIST_MOVE_DOWN, N_("Move song down") },
	{ 0, CMD_ADD, NULL },
	{ 0, CMD_SAVE_PLAYLIST, NULL },
	{ 0, CMD_SCREEN_UPDATE, N_("Center") },
	{ 0, CMD_SELECT_PLAYING, NULL },
	{ 0, CMD_TOGGLE_AUTOCENTER, NULL },

	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Browse screen") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_PLAY, N_("Enter directory/Select and play song") },
	{ 0, CMD_SELECT, NULL },
	{ 0, CMD_ADD, N_("Append song to playlist") },
	{ 0, CMD_SAVE_PLAYLIST, NULL },
	{ 0, CMD_DELETE, N_("Delete playlist") },
	{ 0, CMD_GO_PARENT_DIRECTORY, NULL },
	{ 0, CMD_GO_ROOT_DIRECTORY, NULL },
	{ 0, CMD_SCREEN_UPDATE, NULL },

#ifdef ENABLE_SEARCH_SCREEN
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Search screen") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_SCREEN_SEARCH, N_("Search") },
	{ 0, CMD_PLAY, N_("Select and play") },
	{ 0, CMD_SELECT, NULL },
	{ 0, CMD_ADD, N_("Append song to playlist") },
	{ 0, CMD_SELECT_ALL,	 NULL },
	{ 0, CMD_SEARCH_MODE, NULL },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Lyrics screen") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_SCREEN_LYRICS, N_("View Lyrics") },
	{ 0, CMD_SELECT, N_("(Re)load lyrics") },
	/* to translators: this hotkey aborts the retrieval of lyrics
	   from the server */
	{ 0, CMD_INTERRUPT, N_("Interrupt retrieval") },
	{ 0, CMD_LYRICS_UPDATE, N_("Download lyrics for currently playing song") },
	{ 0, CMD_SAVE_PLAYLIST, N_("Save lyrics") },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Outputs screen") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_PLAY, N_("Enable/disable output") },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ 0, CMD_NONE, NULL },
	{ 0, CMD_NONE, NULL },
	{ 1, CMD_NONE, N_("Keydef screen") },
	{ 2, CMD_NONE, NULL },
	{ 0, CMD_PLAY, N_("Edit keydefs for selected command") },
	{ 0, CMD_DELETE, N_("Remove selected keydef") },
	{ 0, CMD_GO_PARENT_DIRECTORY, N_("Go up a level") },
	{ 0, CMD_SAVE_PLAYLIST, N_("Apply and save changes") },
#endif
};

static struct list_window *lw;

static const char *
list_callback(unsigned i, G_GNUC_UNUSED void *data)
{
	const struct help_text_row *row = &help_text[i];

	assert(i < G_N_ELEMENTS(help_text));

	if (row->text != NULL)
		return _(row->text);

	if (row->command != CMD_NONE)
		return get_key_description(row->command);

	return "";
}

static void
help_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
	lw->hide_cursor = true;
	list_window_set_length(lw, G_N_ELEMENTS(help_text));
}

static void
help_resize(int cols, int rows)
{
	list_window_resize(lw, cols, rows);
}

static void
help_exit(void)
{
  list_window_free(lw);
}


static const char *
help_title(G_GNUC_UNUSED char *str, G_GNUC_UNUSED size_t size)
{
	return _("Help");
}

static void
screen_help_paint_callback(WINDOW *w, unsigned i,
			   unsigned y, unsigned width,
			   G_GNUC_UNUSED bool selected,
			   G_GNUC_UNUSED void *data)
{
	const struct help_text_row *row = &help_text[i];

	assert(i < G_N_ELEMENTS(help_text));

	row_color(w, row->highlight ? COLOR_LIST_BOLD : COLOR_LIST, false);

	wclrtoeol(w);

	if (row->command == CMD_NONE) {
		if (row->text != NULL)
			mvwaddstr(w, y, 6, row->text);
		else if (row->highlight == 2)
			mvwhline(w, y, 3, '-', width - 6);
	} else {
		const char *key = get_key_names(row->command, true);

		if (utf8_width(key) < 20)
			wmove(w, y, 20 - utf8_width(key));
		waddstr(w, key);
		mvwaddch(w, y, 21, ':');
		mvwaddstr(w, y, 23,
			  row->text != NULL
			  ? _(row->text)
			  : get_key_description(row->command));
	}
}

static void
help_paint(void)
{
	list_window_paint2(lw, screen_help_paint_callback, NULL);
}

static bool
help_cmd(G_GNUC_UNUSED struct mpdclient *c, command_t cmd)
{
	if (list_window_scroll_cmd(lw, cmd)) {
		help_paint();
		wrefresh(lw->w);
		return true;
	}

	list_window_set_cursor(lw, lw->start);
	if (screen_find(lw,  cmd, list_callback, NULL)) {
		/* center the row */
		list_window_center(lw, lw->selected);
		help_paint();
		wrefresh(lw->w);
		return true;
	}

	return false;
}

const struct screen_functions screen_help = {
	.init = help_init,
	.exit = help_exit,
	.resize = help_resize,
	.paint = help_paint,
	.cmd = help_cmd,
	.get_title = help_title,
};
