/*
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
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

#include "i18n.h"
#include "screen.h"
#include "screen_utils.h"
#include "charset.h"

static list_window_t *lw;

struct {
	struct mpd_song *song;
	GPtrArray *lines;
} current;

static void
screen_song_clear(void)
{
	for (guint i = 0; i < current.lines->len; ++i)
		g_free(g_ptr_array_index(current.lines, i));

	g_ptr_array_set_size(current.lines, 0);

	if (current.song != NULL) {
		mpd_freeSong(current.song);
		current.song = NULL;
	}
}

static void
screen_song_paint(void);

/**
 * Repaint and update the screen.
 */
static void
screen_song_repaint(void)
{
	screen_song_paint();
	wrefresh(lw->w);
}

static const char *
screen_song_list_callback(unsigned idx, G_GNUC_UNUSED int *highlight,
			  G_GNUC_UNUSED void *data)
{
	static char buffer[256];
	char *value;

	if (idx >= current.lines->len)
		return NULL;

	value = utf8_to_locale(g_ptr_array_index(current.lines, idx));
	g_strlcpy(buffer, value, sizeof(buffer));
	g_free(value);

	return buffer;
}


static void
screen_song_init(WINDOW *w, int cols, int rows)
{
	current.lines = g_ptr_array_new();
	lw = list_window_init(w, cols, rows);
	lw->flags = LW_HIDE_CURSOR;
}

static void
screen_song_exit(void)
{
	list_window_free(lw);

	screen_song_clear();

	g_ptr_array_free(current.lines, TRUE);
	current.lines = NULL;
}

static void
screen_song_resize(int cols, int rows)
{
	lw->cols = cols;
	lw->rows = rows;
}

static const char *
screen_song_title(G_GNUC_UNUSED char *str, G_GNUC_UNUSED size_t size)
{
	return _("Song viewer");
}

static void
screen_song_paint(void)
{
	list_window_paint(lw, screen_song_list_callback, NULL);
}

static bool
screen_song_cmd(mpdclient_t *c, command_t cmd)
{
	if (list_window_scroll_cmd(lw, current.lines->len, cmd)) {
		screen_song_repaint();
		return true;
	}

	switch(cmd) {
	case CMD_LOCATE:
		if (current.song != NULL) {
			screen_file_goto_song(c, current.song);
			return true;
		}

		return false;

	default:
		break;
	}

	if (screen_find(lw, current.lines->len,
			cmd, screen_song_list_callback, NULL)) {
		/* center the row */
		list_window_center(lw, current.lines->len, lw->selected);
		screen_song_repaint();
		return true;
	}

	return false;
}

const struct screen_functions screen_song = {
	.init = screen_song_init,
	.exit = screen_song_exit,
	.resize = screen_song_resize,
	.paint = screen_song_paint,
	.cmd = screen_song_cmd,
	.get_title = screen_song_title,
};

static void
screen_song_append(const char *label, const char *value)
{
	assert(label != NULL);

	if (value != NULL)
		g_ptr_array_add(current.lines,
				g_strdup_printf("%s: %s", label, value));
}

void
screen_song_switch(struct mpdclient *c, const struct mpd_song *song)
{
	assert(song != NULL);
	assert(song->file != NULL);

	screen_song_clear();

	current.song = mpd_songDup(song);

	g_ptr_array_add(current.lines, g_strdup(song->file));

	screen_song_append(_("Artist"), song->artist);
	screen_song_append(_("Title"), song->title);
	screen_song_append(_("Album"), song->album);
	screen_song_append(_("Composer"), song->composer);
	screen_song_append(_("Name"), song->name);
	screen_song_append(_("Disc"), song->disc);
	screen_song_append(_("Track"), song->track);
	screen_song_append(_("Date"), song->date);
	screen_song_append(_("Genre"), song->genre);
	screen_song_append(_("Comment"), song->comment);

	screen_switch(&screen_song, c);
}
