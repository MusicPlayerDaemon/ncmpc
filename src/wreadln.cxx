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

#include "wreadln.hxx"
#include "Completion.hxx"
#include "charset.hxx"
#include "screen_utils.hxx"
#include "Point.hxx"
#include "config.h"
#include "util/StringUTF8.hxx"

#include <string>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#if (defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)) && !defined(_WIN32)
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
#define KEY_CTRL_W   23
#define KEY_CTRL_Z   26
#define KEY_BCKSPC   8
#define TAB          9

struct wreadln {
	/** the ncurses window where this field is displayed */
	WINDOW *const w;

	/** the origin coordinates in the window */
	Point point;

	/** the screen width of the input field */
	unsigned width;

	/** is the input masked, i.e. characters displayed as '*'? */
	const bool masked;

	/** the byte position of the cursor */
	size_t cursor = 0;

	/** the byte position displayed at the origin (for horizontal
	    scrolling) */
	size_t start = 0;

	/** the current value */
	std::string value;

	wreadln(WINDOW *_w, bool _masked)
		:w(_w), masked(_masked) {}
};

/** max items stored in the history list */
static const guint wrln_max_history_length = 32;

/** converts a byte position to a screen column */
gcc_pure
static unsigned
byte_to_screen(const char *data, size_t x)
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	assert(x <= strlen(data));

	const std::string partial(data, x);
	return utf8_width(LocaleToUtf8(partial.c_str()).c_str());
#else
	(void)data;

	return (unsigned)x;
#endif
}

/** finds the first character which doesn't fit on the screen */
gcc_pure
static size_t
screen_to_bytes(const char *data, unsigned width)
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	std::string dup(data);

	while (true) {
		unsigned p_width = locale_width(dup.c_str());
		if (p_width <= width)
			return dup.length();

		dup.pop_back();
	}
#else
	(void)data;

	return (size_t)width;
#endif
}

/** returns the screen column where the cursor is located */
gcc_pure
static unsigned
cursor_column(const struct wreadln *wr)
{
	return byte_to_screen(wr->value.data() + wr->start,
			      wr->cursor - wr->start);
}

/** returns the offset in the string to align it at the right border
    of the screen */
gcc_pure
static inline size_t
right_align_bytes(const char *data, size_t right, unsigned width)
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	size_t start = 0;

	assert(right <= strlen(data));

	const std::string dup(data, right);

	while (start < right) {
		char *p = locale_to_utf8(dup.c_str() + start);
		unsigned p_width = utf8_width(p);

		if (p_width < width) {
			g_free(p);
			break;
		}

		gunichar c = g_utf8_get_char(p);
		p[g_unichar_to_utf8(c, nullptr)] = 0;

		start += strlen(Utf8ToLocale(p).c_str());
		g_free(p);
	}

	return start;
#else
	(void)data;

	return right >= width ? right + 1 - width : 0;
#endif
}

/** returns the size (in bytes) of the next character */
gcc_pure
static inline size_t
next_char_size(const char *data)
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	char *p = locale_to_utf8(data);

	gunichar c = g_utf8_get_char(p);
	p[g_unichar_to_utf8(c, nullptr)] = 0;
	size_t size = strlen(Utf8ToLocale(p).c_str());
	g_free(p);

	return size;
#else
	(void)data;

	return 1;
#endif
}

/** returns the size (in bytes) of the previous character */
gcc_pure
static inline size_t
prev_char_size(const char *data, size_t x)
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	assert(x > 0);

	char *p = locale_to_utf8(data);

	char *q = p;
	while (true) {
		gunichar c = g_utf8_get_char(q);
		size_t size = g_unichar_to_utf8(c, nullptr);
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
	if (wr->cursor == wr->value.length())
		return;

	size_t size = next_char_size(wr->value.data() + wr->cursor);
	wr->cursor += size;
	if (cursor_column(wr) >= wr->width)
		wr->start = right_align_bytes(wr->value.c_str(),
					      wr->cursor, wr->width);
}

/* move the cursor one step to the left */
static inline void cursor_move_left(struct wreadln *wr)
{
	if (wr->cursor == 0)
		return;

	size_t size = prev_char_size(wr->value.c_str(), wr->cursor);
	assert(wr->cursor >= size);
	wr->cursor -= size;
	if (wr->cursor < wr->start)
		wr->start = wr->cursor;
}

/* move the cursor to the end of the line */
static inline void cursor_move_to_eol(struct wreadln *wr)
{
	wr->cursor = wr->value.length();
	if (cursor_column(wr) >= wr->width)
		wr->start = right_align_bytes(wr->value.c_str(),
					      wr->cursor, wr->width);
}

/* draw line buffer and update cursor position */
static inline void drawline(const struct wreadln *wr)
{
	wmove(wr->w, wr->point.y, wr->point.x);
	/* clear input area */
	whline(wr->w, ' ', wr->width);
	/* print visible part of the line buffer */
	if (wr->masked)
		whline(wr->w, '*', utf8_width(wr->value.c_str() + wr->start));
	else
		waddnstr(wr->w, wr->value.c_str() + wr->start,
			 screen_to_bytes(wr->value.c_str(), wr->width));
	/* move the cursor to the correct position */
	wmove(wr->w, wr->point.y, wr->point.x + cursor_column(wr));
	/* tell ncurses to redraw the screen */
	doupdate();
}

#if (defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)) && !defined(_WIN32)
static bool
multibyte_is_complete(const char *p, size_t length)
{
	char *q = g_locale_to_utf8(p, length,
				   nullptr, nullptr, nullptr);
	if (q != nullptr) {
		g_free(q);
		return true;
	} else {
		return false;
	}
}
#endif

static void
wreadln_insert_byte(struct wreadln *wr, gint key)
{
	size_t length = 1;
#if (defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)) && !defined(_WIN32)
	char buffer[32] = { (char)key };
	struct pollfd pfd = {
		.fd = 0,
		.events = POLLIN,
		.revents = 0,
	};

	/* wide version: try to complete the multibyte sequence */

	while (length < sizeof(buffer)) {
		if (multibyte_is_complete(buffer, length))
			/* sequence is complete */
			break;

		/* poll for more bytes on stdin, without timeout */

		if (poll(&pfd, 1, 0) <= 0)
			/* no more input from keyboard */
			break;

		buffer[length++] = wgetch(wr->w);
	}

	wr->value.insert(wr->cursor, buffer, length);

#else
	wr->value.insert(wr->cursor, key);
#endif

	wr->cursor += length;
	if (cursor_column(wr) >= wr->width)
		wr->start = right_align_bytes(wr->value.c_str(),
					      wr->cursor, wr->width);
}

static void
wreadln_delete_char(struct wreadln *wr, size_t x)
{
	assert(x < wr->value.length());

	size_t length = next_char_size(&wr->value[x]);
	wr->value.erase(x, length);
}

/* libcurses version */

static std::string
_wreadln(WINDOW *w,
	 const char *prompt,
	 const char *initial_value,
	 unsigned x1,
	 History *history,
	 Completion *completion,
	 bool masked)
{
	struct wreadln wr(w, masked);
	History::iterator hlist, hcurrent;

#ifdef NCMPC_MINI
	(void)completion;
#endif

	/* turn off echo */
	noecho();
	/* make sure the cursor is visible */
	curs_set(1);
	/* print prompt string */
	if (prompt) {
		waddstr(w, prompt);
		waddstr(w, ": ");
	}
	/* retrieve y and x0 position */
	getyx(w, wr.point.y, wr.point.x);
	/* check the x1 value */
	if (x1 <= (unsigned)wr.point.x || x1 > (unsigned)COLS)
		x1 = COLS;
	wr.width = x1 - wr.point.x;
	/* clear input area */
	mvwhline(w, wr.point.y, wr.point.x, ' ', wr.width);

	if (history) {
		/* append the a new line to our history list */
		history->emplace_back();
		/* hlist points to the current item in the history list */
		hcurrent = hlist = std::prev(history->end());
	}

	if (initial_value == (char *)-1) {
		/* get previous history entry */
		if (history && hlist != history->begin()) {
			/* get previous line */
			--hlist;
			wr.value = *hlist;
		}
		cursor_move_to_eol(&wr);
		drawline(&wr);
	} else if (initial_value) {
		/* copy the initial value to the line buffer */
		wr.value = initial_value;
		cursor_move_to_eol(&wr);
		drawline(&wr);
	}

	gint key = 0;
	while (key != 13 && key != '\n') {
		key = wgetch(w);

		/* check if key is a function key */
		for (size_t i = 0; i < 63; i++)
			if (key == (int)KEY_F(i)) {
				key = KEY_F(1);
				i = 64;
			}

		switch (key) {
#ifdef HAVE_GETMOUSE
		case KEY_MOUSE: /* ignore mouse events */
#endif
		case ERR: /* ignore errors */
			break;

		case TAB:
#ifndef NCMPC_MINI
			if (completion != nullptr) {
				completion->Pre(wr.value.c_str());
				auto r = completion->Complete(wr.value.c_str());
				if (!r.new_prefix.empty()) {
					wr.value = std::move(r.new_prefix);
					cursor_move_to_eol(&wr);
				} else
					screen_bell();

				completion->Post(wr.value.c_str(), r.range);
			}
#endif
			break;

		case KEY_CTRL_G:
			screen_bell();
			if (history) {
				history->pop_back();
			}
			return {};

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
			wr.value.erase(wr.cursor);
			break;
		case KEY_CTRL_U:
			wr.value.erase(0, wr.cursor);
			wr.cursor = 0;
			break;
		case KEY_CTRL_W:
			/* Firstly remove trailing spaces. */
			for (; wr.cursor > 0 && wr.value[wr.cursor - 1] == ' ';)
			{
				cursor_move_left(&wr);
				wreadln_delete_char(&wr, wr.cursor);
			}
			/* Then remove word until next space. */
			for (; wr.cursor > 0 && wr.value[wr.cursor - 1] != ' ';)
			{
				cursor_move_left(&wr);
				wreadln_delete_char(&wr, wr.cursor);
			}
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
			if (wr.cursor < wr.value.length())
				wreadln_delete_char(&wr, wr.cursor);
			break;
		case KEY_UP:
		case KEY_CTRL_P:
			/* get previous history entry */
			if (history && hlist != history->begin()) {
				if (hlist == hcurrent)
					/* save the current line */
					*hlist = wr.value;

				/* get previous line */
				--hlist;
				wr.value = *hlist;
			}
			cursor_move_to_eol(&wr);
			break;
		case KEY_DOWN:
		case KEY_CTRL_N:
			/* get next history entry */
			if (history && std::next(hlist) != history->end()) {
				/* get next line */
				++hlist;
				wr.value = *hlist;
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
		if (!wr.value.empty()) {
			/* update the current history entry */
			*hcurrent = wr.value;
		} else {
			/* the line was empty - remove the current history entry */
			history->erase(hcurrent);
		}

		auto history_length = history->size();
		while (history_length > wrln_max_history_length) {
			history->pop_front();
			--history_length;
		}
	}

	return std::move(wr.value);
}

std::string
wreadln(WINDOW *w,
	const char *prompt,
	const char *initial_value,
	unsigned x1,
	History *history,
	Completion *completion)
{
	return  _wreadln(w, prompt, initial_value, x1,
			 history, completion, false);
}

std::string
wreadln_masked(WINDOW *w,
	       const char *prompt,
	       const char *initial_value,
	       unsigned x1)
{
	return  _wreadln(w, prompt, initial_value, x1, nullptr, nullptr, true);
}
