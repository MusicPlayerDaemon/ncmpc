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

#include "ListPage.hxx"

#include <vector>
#include <string>

struct mpdclient;
class ScreenManager;

class TextPage : public ListPage {
protected:
	ScreenManager &screen;

	std::vector<std::string> lines;

public:
	TextPage(ScreenManager &_screen,
		 WINDOW *w, unsigned cols, unsigned rows)
		:ListPage(w, cols, rows), screen(_screen) {
		lw.hide_cursor = true;
	}

protected:
	bool IsEmpty() const {
		return lines.empty();
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
	void Paint() const override {
		lw.Paint(ListCallback, const_cast<TextPage *>(this));
	}

	bool OnCommand(struct mpdclient &c, command_t cmd) override;
};

#endif
