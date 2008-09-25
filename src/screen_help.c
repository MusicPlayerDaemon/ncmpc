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
#include "ncmpc.h"
#include "mpdclient.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "gcc.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>


typedef struct {
	signed char highlight;
	command_t command;
	const char *text;
} help_text_row_t;

static help_text_row_t help_text[] =
{
  { 1, CMD_NONE,           N_("Keys - Movement") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_LIST_PREVIOUS,  NULL },
  { 0, CMD_LIST_NEXT,      NULL },
  { 0, CMD_LIST_PREVIOUS_PAGE, NULL }, 
  { 0, CMD_LIST_NEXT_PAGE, NULL },
  { 0, CMD_LIST_FIRST,     NULL },
  { 0, CMD_LIST_LAST,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_PREVIOUS,NULL },
  { 0, CMD_SCREEN_NEXT,    NULL },
  { 0, CMD_SCREEN_HELP,    NULL },
  { 0, CMD_SCREEN_PLAY,    NULL },
  { 0, CMD_SCREEN_FILE,    NULL },
#ifdef ENABLE_SEARCH_SCREEN
  { 0, CMD_SCREEN_SEARCH,  NULL },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
  { 0, CMD_SCREEN_KEYDEF,  NULL },
#endif

  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Global") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_STOP,           NULL },
  { 0, CMD_PAUSE,          NULL },
  { 0, CMD_CROP, NULL },
  { 0, CMD_TRACK_NEXT,     NULL },
  { 0, CMD_TRACK_PREVIOUS, NULL },
  { 0, CMD_SEEK_FORWARD,   NULL },
  { 0, CMD_SEEK_BACKWARD,  NULL },
  { 0, CMD_VOLUME_DOWN,    NULL },
  { 0, CMD_VOLUME_UP,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_REPEAT,         NULL },
  { 0, CMD_RANDOM,         NULL },
  { 0, CMD_CROSSFADE,      NULL },
  { 0, CMD_SHUFFLE,        NULL },
  { 0, CMD_DB_UPDATE,      NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_LIST_FIND,      NULL },
  { 0, CMD_LIST_RFIND,     NULL },
  { 0, CMD_LIST_FIND_NEXT, NULL },
  { 0, CMD_LIST_RFIND_NEXT,  NULL },
  { 0, CMD_TOGGLE_FIND_WRAP, NULL },
  { 0, CMD_NONE,           NULL },
  { 0, CMD_QUIT,           NULL },

  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Playlist screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_PLAY,           N_("Play") },
  { 0, CMD_DELETE,         NULL },
  { 0, CMD_CLEAR,          NULL },
  { 1, CMD_LIST_MOVE_UP,   N_("Move song up") },
  { 0, CMD_LIST_MOVE_DOWN, N_("Move song down") },
  { 0, CMD_ADD,            NULL },
  { 0, CMD_SAVE_PLAYLIST,  NULL },
  { 0, CMD_SCREEN_UPDATE,  N_("Center") },
  { 0, CMD_TOGGLE_AUTOCENTER, NULL },

  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Browse screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_PLAY,           N_("Enter directory/Select and play song") },
  { 0, CMD_SELECT,         NULL },
  { 0, CMD_SAVE_PLAYLIST,  NULL },
  { 0, CMD_DELETE,         N_("Delete playlist") },
  { 0, CMD_GO_PARENT_DIRECTORY, NULL },
  { 0, CMD_GO_ROOT_DIRECTORY, NULL },
  { 0, CMD_SCREEN_UPDATE,  NULL },

#ifdef ENABLE_SEARCH_SCREEN
  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Search screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_SEARCH,  N_("Search") },
  { 0, CMD_PLAY,           N_("Select and play") },
  { 0, CMD_SELECT,         NULL },
  { 0, CMD_SELECT_ALL,	   NULL },
  { 0, CMD_SEARCH_MODE,    NULL },
#endif
#ifdef ENABLE_LYRICS_SCREEN
  { 0, CMD_NONE,           NULL },
  { 0, CMD_NONE,           NULL },
  { 1, CMD_NONE,           N_("Keys - Lyrics screen") },
  { 2, CMD_NONE,           NULL },
  { 0, CMD_SCREEN_LYRICS,  N_("View Lyrics") },
  { 0, CMD_SELECT,         N_("(Re)load lyrics") },
  { 0, CMD_INTERRUPT,         N_("Interrupt retrieval") },
  { 0, CMD_LYRICS_UPDATE,         N_("Explicitly download lyrics") },
  { 0, CMD_ADD,         N_("Save lyrics") }, 
#endif
};

#define help_text_rows (sizeof(help_text) / sizeof(help_text[0]))

static list_window_t *lw = NULL;


static const char *
list_callback(unsigned idx, int *highlight, mpd_unused void *data)
{
	static char buf[512];

	if (idx >= help_text_rows)
		return NULL;

	if (help_text[idx].highlight)
		*highlight = 1;

	if (help_text[idx].command == CMD_NONE) {
		if (help_text[idx].text)
			g_snprintf(buf, sizeof(buf), "      %s", _(help_text[idx].text));
		else if (help_text[idx].highlight == 2) {
			int i;

			for (i = 3; i < COLS - 3 && i < (int)sizeof(buf); i++)
				buf[i] = '-';
			buf[i] = '\0';
		} else
			g_strlcpy(buf, " ", sizeof(buf));
		return buf;
	}

	if (help_text[idx].text)
		g_snprintf(buf, sizeof(buf),
			   "%20s : %s   ",
			   get_key_names(help_text[idx].command, TRUE),
			   _(help_text[idx].text));
	else
		g_snprintf(buf, sizeof(buf),
			   "%20s : %s   ",
			   get_key_names(help_text[idx].command, TRUE),
			   get_key_description(help_text[idx].command));
	return buf;
}

static void
help_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
  lw->flags = LW_HIDE_CURSOR;
}

static void
help_resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}

static void
help_exit(void)
{
  list_window_free(lw);
}


static const char *
help_title(mpd_unused char *str, mpd_unused size_t size)
{
	return _("Help");
}

static void
help_paint(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	list_window_paint(lw, list_callback, NULL);
}

static int
help_cmd(screen_t *screen, mpd_unused mpdclient_t *c, command_t cmd)
{
	if (list_window_scroll_cmd(lw, help_text_rows, cmd)) {
		list_window_paint(lw, list_callback, NULL);
		wrefresh(lw->w);
		return 1;
	}

	lw->selected = lw->start+lw->rows;
	if (screen_find(screen,
			lw,  help_text_rows,
			cmd, list_callback, NULL)) {
		/* center the row */
		list_window_center(lw, help_text_rows, lw->selected);
		list_window_paint(lw, list_callback, NULL);
		wrefresh(lw->w);
		return 1;
	}

	return 0;
}

const struct screen_functions screen_help = {
	.init = help_init,
	.exit = help_exit,
	.resize = help_resize,
	.paint = help_paint,
	.cmd = help_cmd,
	.get_title = help_title,
};
