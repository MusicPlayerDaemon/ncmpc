// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Point.hxx"
#include "Size.hxx"

#include <curses.h>

#include <string_view>

enum class Style : unsigned;

/**
 * A non-owning wrapper for a ncurses #WINDOW pointer.
 */
struct Window {
	WINDOW *w;

	explicit constexpr Window(WINDOW *_w) noexcept:w(_w) {}

	[[gnu::pure]]
	const Size GetSize() const noexcept {
		Size size;
		getmaxyx(w, size.height, size.width);
		return size;
	}

	[[gnu::pure]]
	const Point GetCursor() const noexcept {
		Point p;
		getyx(w, p.y, p.x);
		return p;
	}

	[[gnu::pure]]
	unsigned GetWidth() const noexcept {
		return getmaxx(w);
	}

	[[gnu::pure]]
	unsigned GetHeight() const noexcept {
		return getmaxy(w);
	}

	void SetBackgroundStyle(Style style) const noexcept {
		wbkgd(w, COLOR_PAIR(unsigned(style)));
	}

	void Move(Point p) const noexcept {
		mvwin(w, p.y, p.x);
	}

	void AttributeOn(int attrs) const noexcept {
		wattron(w, attrs);
	}

	void AttributeOff(int attrs) const noexcept {
		wattroff(w, attrs);
	}

	void MoveCursor(Point p) const noexcept {
		wmove(w, p.y, p.x);
	}

	void Erase() const noexcept {
		werase(w);
	}

	void ClearToEol() const noexcept {
		wclrtoeol(w);
	}

	void ClearToBottom() const noexcept {
		wclrtobot(w);
	}

	void Char(chtype ch) const noexcept {
		waddch(w, ch);
	}

	void Char(Point p, chtype ch) const noexcept {
		mvwaddch(w, p.y, p.x, ch);
	}

	void String(const char *s) const noexcept {
		waddstr(w, s);
	}

	void String(const std::string_view s) const noexcept {
		waddnstr(w, s.data(), s.size());
	}

	void String(Point p, const char *s) const noexcept {
		mvwaddstr(w, p.y, p.x, s);
	}

	void String(Point p, std::string_view s) const noexcept {
		mvwaddnstr(w, p.y, p.x, s.data(), s.size());
	}

	void HLine(unsigned width, chtype ch) const noexcept {
		whline(w, ch, width);
	}

	void HLine(Point p, unsigned width, chtype ch) const noexcept {
		mvwhline(w, p.y, p.x, ch, width);
	}

#if NCURSES_WIDECHAR
	void Char(const cchar_t *ch) const noexcept {
		wadd_wch(w, ch);
	}

	void Char(Point p, const cchar_t *ch) const noexcept {
		mvwadd_wch(w, p.y, p.x, ch);
	}

	void HLine(unsigned width, const cchar_t *ch) const noexcept {
		whline_set(w, ch, width);
	}

	void HLine(Point p, unsigned width, const cchar_t *ch) const noexcept {
		mvwhline_set(w, p.y, p.x, ch, width);
	}
#endif // NCURSES_WIDECHAR

	void RefreshNoOut() const noexcept {
		wnoutrefresh(w);
	}

	void Refresh() const noexcept {
		wrefresh(w);
	}

	int GetChar() const noexcept {
		return wgetch(w);
	}
};

/**
 * Like #Window, but this class owns the #WINDOW.  The constructor
 * creates a new instance and the destructor deletes it.
 */
struct UniqueWindow : Window {
	UniqueWindow(Point p, Size _size) noexcept
		:Window(newwin(_size.height, _size.width, p.y, p.x)) {}

	~UniqueWindow() noexcept {
		delwin(w);
	}

	UniqueWindow(const UniqueWindow &) = delete;
	UniqueWindow &operator=(const UniqueWindow &) = delete;

	void Resize(Size new_size) noexcept {
		wresize(w, new_size.height, new_size.width);
	}
};
