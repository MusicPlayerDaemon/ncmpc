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

#ifndef NCMPC_LIST_PAGE_HXX
#define NCMPC_LIST_PAGE_HXX

#include "Page.hxx"
#include "list_window.hxx"

/**
 * An abstract #Page implementation which shows a #ListWindow.
 */
class ListPage : public Page {
protected:
	ListWindow lw;

public:
	ListPage(WINDOW *w, unsigned cols, unsigned rows)
		:lw(w, cols, rows) {}

public:
	/* virtual methods from class Page */
	void OnResize(unsigned cols, unsigned rows) override {
		list_window_resize(&lw, cols, rows);
	}

	bool OnCommand(struct mpdclient &, command_t cmd) override {
		if (lw.hide_cursor
		    ? list_window_scroll_cmd(&lw, cmd)
		    : list_window_cmd(&lw, cmd)) {
			SetDirty();
			return true;
		}

		return false;
	}

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &, int, int y,
		     mmask_t bstate) override {
		if (list_window_mouse(&lw, bstate, y)) {
			SetDirty();
			return true;
		}

		return false;
	}

#endif
};

#endif
