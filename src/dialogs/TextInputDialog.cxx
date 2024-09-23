// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TextInputDialog.hxx"
#include "ui/Bell.hxx"
#include "ui/Keys.hxx"
#include "ui/Options.hxx"
#include "ui/Window.hxx"
#include "util/LocaleString.hxx"
#include "Completion.hxx"
#include "Styles.hxx"

#ifndef _WIN32
#include "WaitUserInput.hxx"
#endif

#include <cassert>

#include <string.h>

using std::string_view_literals::operator""sv;

/** max items stored in the history list */
static constexpr std::size_t wrln_max_history_length = 32;

inline void
TextInputDialog::SetReady() noexcept
{
	assert(!ready);
	ready = true;

	/* update history */
	if (history) {
		if (!value.empty()) {
			/* update the current history entry */
			*hcurrent = value;
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

	if (continuation)
		continuation.resume();
}

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

inline unsigned
TextInputDialog::GetCursorColumn() const noexcept
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

inline void
TextInputDialog::MoveCursorRight() noexcept
{
	if (cursor == value.length())
		return;

	size_t size = CharSizeMB(value.substr(cursor));
	cursor += size;
	if (GetCursorColumn() >= width)
		start = right_align_bytes(value.c_str(), cursor, width);
}

inline void
TextInputDialog::MoveCursorLeft() noexcept
{
	const char *v = value.c_str();
	const char *new_cursor = PrevCharMB(v, v + cursor);
	cursor = new_cursor - v;
	if (cursor < start)
		start = cursor;
}

inline void
TextInputDialog::MoveCursorToEnd() noexcept
{
	cursor = value.length();
	if (GetCursorColumn() >= width)
		start = right_align_bytes(value.c_str(),
					      cursor, width);
}

inline void
TextInputDialog::InsertByte([[maybe_unused]] const Window window, int key) noexcept
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

	InvokeModifiedCallback();
}

inline void
TextInputDialog::DeleteChar(size_t x) noexcept
{
	assert(x < value.length());

	size_t length = CharSizeMB(value.substr(x));
	value.erase(x, length);

	InvokeModifiedCallback();
}

void
TextInputDialog::OnLeave(const Window window) noexcept
{
	curs_set(0);

	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::STATUS);
}

void
TextInputDialog::OnCancel() noexcept
{
	value.clear();
	SetReady();
}

bool
TextInputDialog::OnKey(const Window window, int key)
{
	if (key == KEY_RETURN || key == KEY_LINEFEED) {
		if (fragile) {
			Cancel();
			return false;
		}

		SetReady();
		return true;
	}

	/* check if key is a function key */
	if (IsFKey(key)) {
		if (fragile)
			Cancel();
		return false;
	}

	if (IsBackspace(key)) {
		if (cursor > 0) { /* - 1 from buf[n+1] to buf   */
			MoveCursorLeft();
			DeleteChar();
		}

		return true;
	}

	switch (key) {
	case KEY_TAB:
#ifndef NCMPC_MINI
		if (completion != nullptr) {
			completion->Pre(value);
			auto r = completion->Complete(value);
			if (!r.new_prefix.empty()) {
				value = std::move(r.new_prefix);
				MoveCursorToEnd();
			} else
				Bell();

			completion->Post(value, r.range);
			return true;
		}
#endif

		if (fragile) {
			Cancel();
			return false;
		}

		break;

	case KEY_CTL('C'):
	case KEY_CTL('G'):
		Bell();
		if (history) {
			history->pop_back();
		}
		Cancel();
		return true;

	case KEY_LEFT:
	case KEY_CTL('B'):
		MoveCursorLeft();
		break;
	case KEY_RIGHT:
	case KEY_CTL('F'):
		MoveCursorRight();
		break;
	case KEY_HOME:
	case KEY_CTL('A'):
		cursor = 0;
		start = 0;
		break;
	case KEY_END:
	case KEY_CTL('E'):
		MoveCursorToEnd();
		break;
	case KEY_CTL('K'):
		value.erase(cursor);
		InvokeModifiedCallback();
		break;
	case KEY_CTL('U'):
		value.erase(0, cursor);
		cursor = 0;
		InvokeModifiedCallback();
		break;
	case KEY_CTL('W'):
		/* Firstly remove trailing spaces. */
		for (; cursor > 0 && value[cursor - 1] == ' ';)
		{
			MoveCursorLeft();
			DeleteChar();
		}
		/* Then remove word until next space. */
		for (; cursor > 0 && value[cursor - 1] != ' ';)
		{
			MoveCursorLeft();
			DeleteChar();
		}
		break;
	case KEY_DC:		/* handle delete key. As above */
	case KEY_CTL('D'):
		if (cursor < value.length())
			DeleteChar();
		break;
	case KEY_UP:
	case KEY_CTL('P'):
		if (fragile) {
			Cancel();
			return false;
		}

		/* get previous history entry */
		if (history && hlist != history->begin()) {
			if (hlist == hcurrent)
				/* save the current line */
				*hlist = value;

			/* get previous line */
			--hlist;
			value = *hlist;
		}
		MoveCursorToEnd();
		break;
	case KEY_DOWN:
	case KEY_CTL('N'):
		if (fragile) {
			Cancel();
			return false;
		}

		/* get next history entry */
		if (history && std::next(hlist) != history->end()) {
			/* get next line */
			++hlist;
			value = *hlist;
		}
		MoveCursorToEnd();
		break;

	case KEY_IC:
	case KEY_PPAGE:
	case KEY_NPAGE:
		/* ignore char */
		break;
	default:
		if (key >= 32 && key <= 0xff)
			InsertByte(window, key);
		else  if (fragile) {
			Cancel();
			return false;
		}
	}

	return true;
}

void
TextInputDialog::Paint(const Window window) const noexcept
{
	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::INPUT);

	SelectStyle(window, Style::STATUS_ALERT);
	window.String({0, 0}, prompt);
	window.String(": "sv);

	point = window.GetCursor();
	width = window.GetWidth() - point.x;

	SelectStyle(window, Style::INPUT);

	/* print visible part of the line buffer */
	if (masked) {
		const unsigned value_width = StringWidthMB(value.substr(start));
		window.HLine(value_width, '*');
		window.MoveCursor(point + Size{value_width, 0});
	} else {
		window.String({value.c_str() + start, screen_to_bytes(value.c_str() + start, width)});
	}

	/* clear the rest */
	window.ClearToEol();
	/* move the cursor to the correct position */
	window.MoveCursor({point.x + (int)GetCursorColumn(), point.y});

	curs_set(1);
}
