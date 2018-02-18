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

#include "screen_interface.hxx"
#include "list_window.hxx"

#include <glib.h>

struct mpdclient;

class TextPage : public Page {
protected:
	ListWindow lw;

	GPtrArray *lines = g_ptr_array_new();

public:
	TextPage(WINDOW *w, unsigned cols, unsigned rows)
		:lw(w, cols, rows) {
		lw.hide_cursor = true;
	}

	~TextPage() override {
		Clear();
		g_ptr_array_free(lines, true);
	}

protected:
	bool IsEmpty() const {
		return lines->len == 0;
	}

	void Clear();

	void Append(const char *str);

	void Set(const char *str) {
		Clear();
		Append(str);
	}

	/**
	 * Repaint and update the screen.
	 */
	void Repaint() {
		Paint();
		wrefresh(lw.w);
	}

private:
	const char *ListCallback(unsigned idx) const;

	static const char *ListCallback(unsigned idx, void *data) {
		const auto &p = *(const TextPage *)data;
		return p.ListCallback(idx);
	}

public:
	/* virtual methods from class Page */
	void OnResize(unsigned cols, unsigned rows) override {
		list_window_resize(&lw, cols, rows);
	}

	void Paint() const override {
		list_window_paint(&lw, ListCallback,
				  const_cast<TextPage *>(this));
	}

	bool OnCommand(struct mpdclient &c, command_t cmd) override;
};

#endif
