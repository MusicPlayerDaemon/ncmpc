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

#include "wreadln.h"
#include "charset.h"
#include "screen_utils.h"
#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#ifdef ENABLE_WIDE
#include <sys/poll.h>
#endif

#define KEY_CTRL_A   1
#define KEY_CTRL_B   2
#define KEY_CTRL_C   3
#define KEY_CTRL_D   4
#define KEY_CTRL_E   5
#define KEY_CTRL_F   6
#define KEY_CTRL_G   7
#define KEY_CTRL_K   11
#define KEY_CTRL_N   14
#define KEY_CTRL_P   16
#define KEY_CTRL_U   21
#define KEY_CTRL_Z   26
#define KEY_BCKSPC   8
#define TAB          9

struct wreadln {
	/** the ncurses window where this field is displayed */
	WINDOW *const w;

	/** the origin coordinates in the window */
	unsigned x, y;

	/** the screen width of the input field */
	unsigned width;

	/** is the input masked, i.e. characters displayed as '*'? */
	const gboolean masked;

	/** the byte position of the cursor */
	size_t cursor;

	/** the byte position displayed at the origin (for horizontal
	    scrolling) */
	size_t start;

	/** the current value */
	gchar line[1024];
};

/** max items stored in the history list */
static const guint wrln_max_history_length = 32;

#ifndef NCMPC_MINI
void *wrln_completion_callback_data = NULL;
wrln_gcmp_pre_cb_t wrln_pre_completion_callback = NULL;
wrln_gcmp_post_cb_t wrln_post_completion_callback = NULL;
#endif

/** converts a byte position to a screen column */
static unsigned
byte_to_screen(const gchar *data, size_t x)
{
#ifdef ENABLE_WIDE
	gchar *dup;
	char *p;
	unsigned width;

	assert(x <= strlen(data));

	dup = g_strdup(data);
	dup[x] = 0;
	p = locale_to_utf8(dup);
	g_free(dup);

	width = utf8_width(p);
	g_free(p);

	return width;
#else
	(void)data;

	return (unsigned)x;
#endif
}

/** finds the first character which doesn't fit on the screen */
static size_t
screen_to_bytes(const gchar *data, unsigned width)
{
#ifdef ENABLE_WIDE
	size_t length = strlen(data);
	gchar *dup = g_strdup(data);
	char *p;
	unsigned p_width;

	while (true) {
		dup[length] = 0;
		p = locale_to_utf8(dup);
		p_width = utf8_width(p);
		g_free(p);
		if (p_width <= width)
			break;

		--length;
	}

	g_free(dup);

	return length;
#else
	(void)data;

	return (size_t)width;
#endif
}

/** returns the screen colum where the cursor is located */
static unsigned
cursor_column(const struct wreadln *wr)
{
	return byte_to_screen(wr->line + wr->start,
			      wr->cursor - wr->start);
}

/** returns the offset in the string to align it at the right border
    of the screen */
static inline size_t
right_align_bytes(const gchar *data, size_t right, unsigned width)
{
#ifdef ENABLE_WIDE
	gchar *dup;
	size_t start = 0;

	assert(right <= strlen(data));

	dup = g_strdup(data);
	dup[right] = 0;

	while (dup[start] != 0) {
		char *p = locale_to_utf8(dup + start), *q;
		unsigned p_width = utf8_width(p);
		gunichar c;

		if (p_width < width) {
			g_free(p);
			break;
		}

		c = g_utf8_get_char(p);
		p[g_unichar_to_utf8(c, NULL)] = 0;
		q = utf8_to_locale(p);
		g_free(p);

		start += strlen(q);
		g_free(q);
	}

	g_free(dup);

	return start;
#else
	(void)data;

	return right >= width ? right + 1 - width : 0;
#endif
}

/** returns the size (in bytes) of the next character */
static inline size_t
next_char_size(const gchar *data)
{
#ifdef ENABLE_WIDE
	char *p = locale_to_utf8(data), *q;
	gunichar c;
	size_t size;

	c = g_utf8_get_char(p);
	p[g_unichar_to_utf8(c, NULL)] = 0;
	q = utf8_to_locale(p);
	g_free(p);

	size = strlen(q);
	g_free(q);

	return size;
#else
	(void)data;

	return 1;
#endif
}

/** returns the size (in bytes) of the previous character */
static inline size_t
prev_char_size(const gchar *data, size_t x)
{
#ifdef ENABLE_WIDE
	char *p = locale_to_utf8(data), *q;
	gunichar c;
	size_t size;

	assert(x > 0);

	q = p;
	while (true) {
		c = g_utf8_get_char(q);
		size = g_unichar_to_utf8(c, NULL);
		if (size > x)
			size = x;
		x -= size;
		if (x == 0) {
			g_free(p);
			return size;
		}

		q += size;
	}
#else
	(void)data;
	(void)x;

	return 1;
#endif
}

/* move the cursor one step to the right */
static inline void cursor_move_right(struct wreadln *wr)
{
	size_t size;

	if (wr->line[wr->cursor] == 0)
		return;

	size = next_char_size(wr->line + wr->cursor);
	wr->cursor += size;
	if (cursor_column(wr) >= wr->width)
		wr->start = right_align_bytes(wr->line, wr->cursor, wr->width);
}

/* move the cursor one step to the left */
static inline void cursor_move_left(struct wreadln *wr)
{
	size_t size;

	if (wr->cursor == 0)
		return;

	size = prev_char_size(wr->line, wr->cursor);
	assert(wr->cursor >= size);
	wr->cursor -= size;
	if (wr->cursor < wr->start)
		wr->start = wr->cursor;
}

/* move the cursor to the end of the line */
static inline void cursor_move_to_eol(struct wreadln *wr)
{
	wr->cursor = strlen(wr->line);
	if (cursor_column(wr) >= wr->width)
		wr->start = right_align_bytes(wr->line, wr->cursor, wr->width);
}

/* draw line buffer and update cursor position */
static inline void drawline(const struct wreadln *wr)
{
	wmove(wr->w, wr->y, wr->x);
	/* clear input area */
	whline(wr->w, ' ', wr->width);
	/* print visible part of the line buffer */
	if (wr->masked)
		whline(wr->w, '*', utf8_width(wr->line + wr->start));
	else
		waddnstr(wr->w, wr->line + wr->start,
			 screen_to_bytes(wr->line, wr->width));
	/* move the cursor to the correct position */
	wmove(wr->w, wr->y, wr->x + cursor_column(wr));
	/* tell ncurses to redraw the screen */
	doupdate();
}

#ifdef ENABLE_WIDE
static bool
multibyte_is_complete(const char *p, size_t length)
{
	GError *error = NULL;
	gchar *q = g_locale_to_utf8(p, length,
				    NULL, NULL, &error);
	if (q != NULL) {
		g_free(q);
		return true;
	} else {
		g_error_free(error);
		return false;
	}
}
#endif

static void
wreadln_insert_byte(struct wreadln *wr, gint key)
{
	size_t rest = strlen(wr->line + wr->cursor) + 1;
#ifdef ENABLE_WIDE
	char buffer[32] = { key };
	size_t length = 1;
	struct pollfd pfd = {
		.fd = 0,
		.events = POLLIN,
	};
	int ret;

	/* wide version: try to complete the multibyte sequence */

	while (length < sizeof(buffer)) {
		if (multibyte_is_complete(buffer, length))
			/* sequence is complete */
			break;

		/* poll for more bytes on stdin, without timeout */

		ret = poll(&pfd, 1, 0);
		if (ret <= 0)
			/* no more input from keyboard */
			break;

		buffer[length++] = wgetch(wr->w);
	}

	memmove(wr->line + wr->cursor + length,
		wr->line + wr->cursor, rest);
	memcpy(wr->line + wr->cursor, buffer, length);

#else
	const size_t length = 1;

	memmove(wr->line + wr->cursor + length,
		wr->line + wr->cursor, rest);
	wr->line[wr->cursor] = key;

#endif

	wr->cursor += length;
	if (cursor_column(wr) >= wr->width)
		wr->start = right_align_bytes(wr->line, wr->cursor, wr->width);
}

static void
wreadln_delete_char(struct wreadln *wr, size_t x)
{
	size_t rest, length;

	assert(x < strlen(wr->line));

	length = next_char_size(&wr->line[x]);
	rest = strlen(&wr->line[x + length]) + 1;
	memmove(&wr->line[x], &wr->line[x + length], rest);
}

/* libcurses version */

static gchar *
_wreadln(WINDOW *w,
	 const gchar *prompt,
	 const gchar *initial_value,
	 unsigned x1,
	 GList **history,
	 GCompletion *gcmp,
	 gboolean masked)
{
	struct wreadln wr = {
		.w = w,
		.masked = masked,
		.cursor = 0,
		.start = 0,
	};
	GList *hlist = NULL, *hcurrent = NULL;
	gint key = 0;
	size_t i;

#ifdef NCMPC_MINI
	(void)gcmp;
#endif

	/* turn off echo */
	noecho();
	/* make shure the cursor is visible */
	curs_set(1);
	/* print prompt string */
	if (prompt)
		waddstr(w, prompt);
	/* retrive y and x0 position */
	getyx(w, wr.y, wr.x);
	/* check the x1 value */
	if (x1 <= wr.x || x1 > (unsigned)COLS)
		x1 = COLS;
	wr.width = x1 - wr.x;
	/* clear input area */
	mvwhline(w, wr.y, wr.x, ' ', wr.width);

	if (history) {
		/* append the a new line to our history list */
		*history = g_list_append(*history, g_malloc0(sizeof(wr.line)));
		/* hlist points to the current item in the history list */
		hlist = g_list_last(*history);
		hcurrent = hlist;
	}

	if (initial_value == (char *)-1) {
		/* get previous history entry */
		if (history && hlist->prev) {
			if (hlist == hcurrent)
				/* save the current line */
				g_strlcpy(hlist->data, wr.line, sizeof(wr.line));

			/* get previous line */
			hlist = hlist->prev;
			g_strlcpy(wr.line, hlist->data, sizeof(wr.line));
		}
		cursor_move_to_eol(&wr);
		drawline(&wr);
	} else if (initial_value) {
		/* copy the initial value to the line buffer */
		g_strlcpy(wr.line, initial_value, sizeof(wr.line));
		cursor_move_to_eol(&wr);
		drawline(&wr);
	}

	while (key != 13 && key != '\n') {
		key = wgetch(w);

		/* check if key is a function key */
		for (i = 0; i < 63; i++)
			if (key == (int)KEY_F(i)) {
				key = KEY_F(1);
				i = 64;
			}

		switch (key) {
#ifdef HAVE_GETMOUSE
		case KEY_MOUSE: /* ignore mouse events */
#endif
		case ERR: /* ingnore errors */
			break;

		case TAB:
#ifndef NCMPC_MINI
			if (gcmp) {
				char *prefix = NULL;
				GList *list;

				if (wrln_pre_completion_callback)
					wrln_pre_completion_callback(gcmp, wr.line,
								     wrln_completion_callback_data);
				list = g_completion_complete(gcmp, wr.line, &prefix);
				if (prefix) {
					g_strlcpy(wr.line, prefix, sizeof(wr.line));
					cursor_move_to_eol(&wr);
					g_free(prefix);
				} else
					screen_bell();

				if (wrln_post_completion_callback)
					wrln_post_completion_callback(gcmp, wr.line, list,
								      wrln_completion_callback_data);
			}
#endif
			break;

		case KEY_CTRL_G:
			screen_bell();
			if (history) {
				g_free(hcurrent->data);
				hcurrent->data = NULL;
				*history = g_list_delete_link(*history, hcurrent);
			}
			return NULL;

		case KEY_LEFT:
		case KEY_CTRL_B:
			cursor_move_left(&wr);
			break;
		case KEY_RIGHT:
		case KEY_CTRL_F:
			cursor_move_right(&wr);
			break;
		case KEY_HOME:
		case KEY_CTRL_A:
			wr.cursor = 0;
			wr.start = 0;
			break;
		case KEY_END:
		case KEY_CTRL_E:
			cursor_move_to_eol(&wr);
			break;
		case KEY_CTRL_K:
			wr.line[wr.cursor] = 0;
			break;
		case KEY_CTRL_U:
			wr.cursor = utf8_width(wr.line);
			for (i = 0; i < wr.cursor; i++)
				wr.line[i] = '\0';
			wr.cursor = 0;
			break;
		case 127:
		case KEY_BCKSPC:	/* handle backspace: copy all */
		case KEY_BACKSPACE:	/* chars starting from curpos */
			if (wr.cursor > 0) { /* - 1 from buf[n+1] to buf   */
				cursor_move_left(&wr);
				wreadln_delete_char(&wr, wr.cursor);
			}
			break;
		case KEY_DC:		/* handle delete key. As above */
		case KEY_CTRL_D:
			if (wr.line[wr.cursor] != 0)
				wreadln_delete_char(&wr, wr.cursor);
			break;
		case KEY_UP:
		case KEY_CTRL_P:
			/* get previous history entry */
			if (history && hlist->prev) {
				if (hlist == hcurrent)
					/* save the current line */
					g_strlcpy(hlist->data, wr.line,
						  sizeof(wr.line));

				/* get previous line */
				hlist = hlist->prev;
				g_strlcpy(wr.line, hlist->data,
					  sizeof(wr.line));
			}
			cursor_move_to_eol(&wr);
			break;
		case KEY_DOWN:
		case KEY_CTRL_N:
			/* get next history entry */
			if (history && hlist->next) {
				/* get next line */
				hlist = hlist->next;
				g_strlcpy(wr.line, hlist->data,
					  sizeof(wr.line));
			}
			cursor_move_to_eol(&wr);
			break;

		case '\n':
		case 13:
		case KEY_IC:
		case KEY_PPAGE:
		case KEY_NPAGE:
		case KEY_F(1):
			/* ignore char */
			break;
		default:
			if (key >= 32)
				wreadln_insert_byte(&wr, key);
		}

		drawline(&wr);
	}

	/* update history */
	if (history) {
		if (strlen(wr.line)) {
			/* update the current history entry */
			size_t size = strlen(wr.line) + 1;
			hcurrent->data = g_realloc(hcurrent->data, size);
			g_strlcpy(hcurrent->data, wr.line, size);
		} else {
			/* the line was empty - remove the current history entry */
			g_free(hcurrent->data);
			hcurrent->data = NULL;
			*history = g_list_delete_link(*history, hcurrent);
		}

		while (g_list_length(*history) > wrln_max_history_length) {
			GList *first = g_list_first(*history);

			/* remove the oldest history entry  */
			g_free(first->data);
			first->data = NULL;
			*history = g_list_delete_link(*history, first);
		}
	}

	if (wr.line[0] == 0)
		return NULL;

	return g_strdup(wr.line);
}

gchar *
wreadln(WINDOW *w,
	const gchar *prompt,
	const gchar *initial_value,
	unsigned x1,
	GList **history,
	GCompletion *gcmp)
{
	return  _wreadln(w, prompt, initial_value, x1, history, gcmp, FALSE);
}

gchar *
wreadln_masked(WINDOW *w,
	       const gchar *prompt,
	       const gchar *initial_value,
	       unsigned x1,
	       GList **history,
	       GCompletion *gcmp)
{
	return  _wreadln(w, prompt, initial_value, x1, history, gcmp, TRUE);
}
