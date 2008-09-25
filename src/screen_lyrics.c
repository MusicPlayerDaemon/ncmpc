/*
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
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

#include <sys/stat.h>
#include "ncmpc.h"
#include "options.h"
#include "mpdclient.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "strfsong.h"
#include "lyrics.h"
#include "gcc.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>

static list_window_t *lw = NULL;

static struct {
	const struct mpd_song *song;

	char *artist, *title;

	struct lyrics_loader *loader;

	GPtrArray *lines;
} current;

static void
screen_lyrics_abort(void)
{
	if (current.loader != NULL) {
		lyrics_free(current.loader);
		current.loader = NULL;
	}

	if (current.artist != NULL) {
		g_free(current.artist);
		current.artist = NULL;
	}

	if (current.title != NULL) {
		g_free(current.title);
		current.artist = NULL;
	}

	current.song = NULL;
}

static void
screen_lyrics_clear(void)
{
	guint i;

	for (i = 0; i < current.lines->len; ++i)
		g_free(g_ptr_array_index(current.lines, i));

	g_ptr_array_set_size(current.lines, 0);
}

static void
lyrics_paint(mpdclient_t *c);

/**
 * Repaint and update the screen.
 */
static void
lyrics_repaint(void)
{
	lyrics_paint(NULL);
	wrefresh(lw->w);
}

/**
 * Repaint and update the screen, if it is currently active.
 */
static void
lyrics_repaint_if_active(void)
{
	if (get_cur_mode_id() == 104) { /* XXX don't use the literal number */
		lyrics_repaint();

		/* XXX repaint the screen title */
	}
}

static void
screen_lyrics_set(const GString *str)
{
	const char *p, *eol, *next;

	screen_lyrics_clear();

	p = str->str;
	while ((eol = strchr(p, '\n')) != NULL) {
		char *line;

		next = eol + 1;

		/* strip whitespace at end */

		while (eol > p && (unsigned char)eol[-1] <= 0x20)
			--eol;

		/* create copy and append it to current.lines*/

		line = g_malloc(eol - p + 1);
		memcpy(line, p, eol - p);
		line[eol - p] = 0;

		g_ptr_array_add(current.lines, line);

		/* reset control characters */

		for (eol = line + (eol - p); line < eol; ++line)
			if ((unsigned char)*line < 0x20)
				*line = ' ';

		p = next;
	}

	if (*p != 0)
		g_ptr_array_add(current.lines, g_strdup(p));

	/* paint new data */

	lyrics_repaint_if_active();
}

static void
screen_lyrics_callback(const GString *result, mpd_unused void *data)
{
	assert(current.loader != NULL);

	if (result != NULL)
		screen_lyrics_set(result);
	else
		screen_status_message (_("No lyrics"));

	lyrics_free(current.loader);
	current.loader = NULL;
}

static void
screen_lyrics_load(struct mpd_song *song)
{
	char buffer[MAX_SONGNAME_LENGTH];

	assert(song != NULL);

	screen_lyrics_abort();
	screen_lyrics_clear();

	current.song = song;

	strfsong(buffer, sizeof(buffer), "%artist%", song);
	current.artist = g_strdup(buffer);

	strfsong(buffer, sizeof(buffer), "%title%", song);
	current.title = g_strdup(buffer);

	current.loader = lyrics_load(current.artist, current.title,
				     screen_lyrics_callback, NULL);
}

static FILE *create_lyr_file(const char *artist, const char *title)
{
	char path[1024];

	snprintf(path, 1024, "%s/.lyrics",
		 getenv("HOME"));
	mkdir(path, S_IRWXU);

	snprintf(path, 1024, "%s/.lyrics/%s - %s.txt",
		 getenv("HOME"), artist, title);

	return fopen(path, "w");
}

static int store_lyr_hd(void)
{
	FILE *lyr_file;
	unsigned i;

	lyr_file = create_lyr_file(current.artist, current.title);
	if (lyr_file == NULL)
		return -1;

	for (i = 0; i < current.lines->len; ++i)
		fprintf(lyr_file, "%s\n",
			(const char*)g_ptr_array_index(current.lines, i));

	fclose(lyr_file);
	return 0;
}

static const char *
list_callback(unsigned idx, mpd_unused int *highlight, mpd_unused void *data)
{
	if (idx >= current.lines->len)
		return NULL;

	return g_ptr_array_index(current.lines, idx);
}


static void
lyrics_screen_init(WINDOW *w, int cols, int rows)
{
	current.lines = g_ptr_array_new();
	lw = list_window_init(w, cols, rows);
	lw->flags = LW_HIDE_CURSOR;
}

static void
lyrics_resize(int cols, int rows)
{
	lw->cols = cols;
	lw->rows = rows;
}

static void
lyrics_exit(void)
{
	list_window_free(lw);

	screen_lyrics_abort();
	screen_lyrics_clear();

	g_ptr_array_free(current.lines, TRUE);
	current.lines = NULL;
}

static void
lyrics_open(mpd_unused screen_t *screen, mpdclient_t *c)
{
	if (c->song != NULL && c->song != current.song)
		screen_lyrics_load(c->song);
}


static const char *
lyrics_title(char *str, size_t size)
{
	if (current.loader != NULL)
		return "Lyrics (loading)";
	else if (current.artist != NULL && current.title != NULL &&
		 current.lines->len > 0) {
		snprintf(str, size, "Lyrics: %s - %s",
			 current.artist, current.title);
		return str;
	} else
		return "Lyrics";
}

static void
lyrics_paint(mpd_unused mpdclient_t *c)
{
	list_window_paint(lw, list_callback, NULL);
}

static int
lyrics_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
	if (list_window_scroll_cmd(lw, current.lines->len, cmd)) {
		lyrics_repaint();
		return 1;
	}

	switch(cmd) {
	case CMD_INTERRUPT:
		if (current.loader != NULL) {
			screen_lyrics_abort();
			screen_lyrics_clear();
		}
		return 1;
	case CMD_ADD:
		if (current.loader == NULL && current.artist != NULL &&
		    current.title != NULL && store_lyr_hd() == 0)
			screen_status_message (_("Lyrics saved!"));
		return 1;
	case CMD_LYRICS_UPDATE:
		if (c->song != NULL) {
			screen_lyrics_load(c->song);
			lyrics_repaint();
		}
		return 1;
	default:
		break;
	}

	lw->selected = lw->start+lw->rows;
	if (screen_find(screen,
			lw, current.lines->len,
			cmd, list_callback, NULL)) {
		/* center the row */
		list_window_center(lw, current.lines->len, lw->selected);
		lyrics_repaint();
		return 1;
	}

	return 0;
}

const struct screen_functions screen_lyrics = {
	.init = lyrics_screen_init,
	.exit = lyrics_exit,
	.open = lyrics_open,
	.close = NULL,
	.resize = lyrics_resize,
	.paint = lyrics_paint,
	.cmd = lyrics_cmd,
	.get_title = lyrics_title,
};
