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

#ifndef SCREEN_TEXT_H
#define SCREEN_TEXT_H

#include "list_window.hxx"

#include <glib.h>

struct mpdclient;

struct screen_text {
	GPtrArray *lines;

	ListWindow *lw;
};

static inline void
screen_text_init(struct screen_text *text, WINDOW *w, unsigned cols, unsigned rows)
{
	text->lines = g_ptr_array_new();

	text->lw = new ListWindow(w, cols, rows);
	text->lw->hide_cursor = true;
}

void
screen_text_clear(struct screen_text *text);

static inline void
screen_text_deinit(struct screen_text *text)
{
	screen_text_clear(text);
	g_ptr_array_free(text->lines, true);

	delete text->lw;
}

static inline void
screen_text_resize(struct screen_text *text, unsigned cols, unsigned rows)
{
	list_window_resize(text->lw, cols, rows);
}

static inline bool
screen_text_is_empty(const struct screen_text *text)
{
	return text->lines->len == 0;
}

void
screen_text_append(struct screen_text *text, const char *str);

static inline void
screen_text_set(struct screen_text *text, const char *str)
{
	screen_text_clear(text);
	screen_text_append(text, str);
}

const char *
screen_text_list_callback(unsigned idx, void *data);

static inline void
screen_text_paint(struct screen_text *text)
{
	list_window_paint(text->lw, screen_text_list_callback, text);
}

/**
 * Repaint and update the screen.
 */
static inline void
screen_text_repaint(struct screen_text *text)
{
	screen_text_paint(text);
	wrefresh(text->lw->w);
}

bool
screen_text_cmd(struct screen_text *text, struct mpdclient *c, command_t cmd);

#endif
