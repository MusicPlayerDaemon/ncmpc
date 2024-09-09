// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "wreadln.hxx"
#include "Completion.hxx"
#include "config.h"
#include "ui/Bell.hxx"
#include "ui/Keys.hxx"
#include "ui/Point.hxx"
#include "ui/Window.hxx"
#include "util/LocaleString.hxx"
#include "util/ScopeExit.hxx"

#include <string>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include "WaitUserInput.hxx"
#include <errno.h>
#endif

struct wreadln {
	/** the ncurses window where this field is displayed */
	const Window window;

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

	wreadln(const Window _window, bool _masked) noexcept
		:window(_window), masked(_masked) {}

	/** draw line buffer and update cursor position */
	void Paint() const noexcept;

	/** returns the screen column where the cursor is located */
	[[gnu::pure]]
	unsigned GetCursorColumn() const noexcept;

	/** move the cursor one step to the right */
	void MoveCursorRight() noexcept;

	/** move the cursor one step to the left */
	void MoveCursorLeft() noexcept;

	/** move the cursor to the end of the line */
	void MoveCursorToEnd() noexcept;

	void InsertByte(int key) noexcept;
	void DeleteChar(size_t x) noexcept;
	void DeleteChar() noexcept {
		DeleteChar(cursor);
	}
};

/** max items stored in the history list */
static constexpr std::size_t wrln_max_history_length = 32;

/** converts a byte position to a screen column */
[[gnu::pure]]
static unsigned
byte_to_screen(const char *data, size_t x) noexcept
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	assert(x <= strlen(data));

	return StringWidthMB({data, x});
#else
	(void)data;

	return (unsigned)x;
#endif
}

/** finds the first character which doesn't fit on the screen */
[[gnu::pure]]
static size_t
screen_to_bytes(const char *data, unsigned width) noexcept
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	size_t length = strlen(data);

	while (true) {
		unsigned p_width = StringWidthMB({data, length});
		if (p_width <= width)
			return length;

		--length;
	}
#else
	(void)data;

	return (size_t)width;
#endif
}

unsigned
wreadln::GetCursorColumn() const noexcept
{
	return byte_to_screen(value.data() + start, cursor - start);
}

/** returns the offset in the string to align it at the right border
    of the screen */
[[gnu::pure]]
static inline size_t
right_align_bytes(const char *data, size_t right, unsigned width) noexcept
{
#if defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)
	size_t start = 0;

	assert(right <= strlen(data));

	while (start < right) {
		if (StringWidthMB({data + start, right - start}) < width)
			break;

		start += CharSizeMB({data + start, right - start});
	}

	return start;
#else
	(void)data;

	return right >= width ? right + 1 - width : 0;
#endif
}

void
wreadln::MoveCursorRight() noexcept
{
	if (cursor == value.length())
		return;

	size_t size = CharSizeMB(value.substr(cursor));
	cursor += size;
	if (GetCursorColumn() >= width)
		start = right_align_bytes(value.c_str(), cursor, width);
}

void
wreadln::MoveCursorLeft() noexcept
{
	const char *v = value.c_str();
	const char *new_cursor = PrevCharMB(v, v + cursor);
	cursor = new_cursor - v;
	if (cursor < start)
		start = cursor;
}

void
wreadln::MoveCursorToEnd() noexcept
{
	cursor = value.length();
	if (GetCursorColumn() >= width)
		start = right_align_bytes(value.c_str(),
					      cursor, width);
}

void
wreadln::Paint() const noexcept
{
	window.MoveCursor(point);
	/* print visible part of the line buffer */
	if (masked)
		window.HLine(StringWidthMB(value.substr(start)), '*');
	else
		window.String({value.c_str() + start, screen_to_bytes(value.c_str() + start, width)});
	/* clear the rest */
	window.ClearToEol();
	/* move the cursor to the correct position */
	window.MoveCursor({point.x + (int)GetCursorColumn(), point.y});
	/* tell ncurses to redraw the screen */
	doupdate();
}

void
wreadln::InsertByte(int key) noexcept
{
	size_t length = 1;
#if (defined(HAVE_CURSES_ENHANCED) || defined(ENABLE_MULTIBYTE)) && !defined(_WIN32)
	char buffer[32] = { (char)key };
	WaitUserInput wui;

	/* wide version: try to complete the multibyte sequence */

	while (length < sizeof(buffer)) {
		if (!IsIncompleteCharMB({buffer, length}))
			/* sequence is complete */
			break;

		/* poll for more bytes on stdin, without timeout */

		if (!wui.IsReady())
			/* no more input from keyboard */
			break;

		buffer[length++] = window.GetChar();
	}

	value.insert(cursor, buffer, length);

#else
	value.insert(cursor, key);
#endif

	cursor += length;
	if (GetCursorColumn() >= width)
		start = right_align_bytes(value.c_str(), cursor, width);
}

void
wreadln::DeleteChar(size_t x) noexcept
{
	assert(x < value.length());

	size_t length = CharSizeMB(value.substr(x));
	value.erase(x, length);
}

/* libcurses version */

static std::string
_wreadln(const Window window,
	 const char *initial_value,
	 History *history,
	 Completion *completion,
	 bool masked) noexcept
{
	struct wreadln wr{window, masked};
	History::iterator hlist, hcurrent;

#ifdef NCMPC_MINI
	(void)completion;
#endif

	/* make sure the cursor is visible */
	curs_set(1);
	AtScopeExit() { curs_set(0); };
	/* retrieve y and x0 position */
	wr.point = window.GetCursor();
	wr.width = window.GetWidth() - wr.point.x;

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
	} else if (initial_value) {
		/* copy the initial value to the line buffer */
		wr.value = initial_value;
	}

	wr.MoveCursorToEnd();
	wr.Paint();

#ifndef _WIN32
	WaitUserInput wui;
#endif

	int key = 0;
	while (key != 13 && key != '\n') {
		key = window.GetChar();

#ifndef _WIN32
		if (key == ERR && errno == EAGAIN) {
			if (wui.Wait())
				continue;
			else
				break;
		}
#endif

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
				completion->Pre(wr.value);
				auto r = completion->Complete(wr.value);
				if (!r.new_prefix.empty()) {
					wr.value = std::move(r.new_prefix);
					wr.MoveCursorToEnd();
				} else
					Bell();

				completion->Post(wr.value, r.range);
			}
#endif
			break;

		case KEY_CTRL_C:
		case KEY_CTRL_G:
			Bell();
			if (history) {
				history->pop_back();
			}
			return {};

		case KEY_LEFT:
		case KEY_CTRL_B:
			wr.MoveCursorLeft();
			break;
		case KEY_RIGHT:
		case KEY_CTRL_F:
			wr.MoveCursorRight();
			break;
		case KEY_HOME:
		case KEY_CTRL_A:
			wr.cursor = 0;
			wr.start = 0;
			break;
		case KEY_END:
		case KEY_CTRL_E:
			wr.MoveCursorToEnd();
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
				wr.MoveCursorLeft();
				wr.DeleteChar();
			}
			/* Then remove word until next space. */
			for (; wr.cursor > 0 && wr.value[wr.cursor - 1] != ' ';)
			{
				wr.MoveCursorLeft();
				wr.DeleteChar();
			}
			break;
		case 127:
		case KEY_BCKSPC:	/* handle backspace: copy all */
		case KEY_BACKSPACE:	/* chars starting from curpos */
			if (wr.cursor > 0) { /* - 1 from buf[n+1] to buf   */
				wr.MoveCursorLeft();
				wr.DeleteChar();
			}
			break;
		case KEY_DC:		/* handle delete key. As above */
		case KEY_CTRL_D:
			if (wr.cursor < wr.value.length())
				wr.DeleteChar();
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
			wr.MoveCursorToEnd();
			break;
		case KEY_DOWN:
		case KEY_CTRL_N:
			/* get next history entry */
			if (history && std::next(hlist) != history->end()) {
				/* get next line */
				++hlist;
				wr.value = *hlist;
			}
			wr.MoveCursorToEnd();
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
				wr.InsertByte(key);
		}

		wr.Paint();
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
wreadln(const Window window,
	const char *initial_value,
	History *history,
	Completion *completion) noexcept
{
	return _wreadln(window, initial_value,
			history, completion, false);
}

std::string
wreadln_masked(const Window window,
	       const char *initial_value) noexcept
{
	return  _wreadln(window, initial_value, nullptr, nullptr, true);
}
