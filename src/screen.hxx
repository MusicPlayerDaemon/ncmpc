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

#ifndef SCREEN_H
#define SCREEN_H

#include "config.h"
#include "command.hxx"
#include "Window.hxx"
#include "TitleBar.hxx"
#include "ProgressBar.hxx"
#include "StatusBar.hxx"
#include "Point.hxx"
#include "ncmpc_curses.h"

#include <mpd/client.h>

#include <glib.h>

#include <memory>
#include <map>

struct mpdclient;
struct screen_functions;
class Page;

class ScreenManager {
	struct Layout {
		Size size;

		static constexpr int title_y = 0, title_x = 0;
		static constexpr int main_y = TitleBar::GetHeight(), main_x = 0;
		static constexpr int progress_x = 0;
		static constexpr int status_x = 0;

		constexpr explicit Layout(Size _size)
			:size(_size) {}

		constexpr unsigned GetMainRows() const {
			return GetProgressY() - main_y;
		}

		constexpr Size GetMainSize() const {
			return {size.width, GetMainRows()};
		}

		constexpr int GetProgressY() const {
			return GetStatusY() - 1;
		}

		constexpr int GetStatusY() const {
			return size.height - 1;
		}
	};

	Layout layout;

public:
	TitleBar title_bar;
	Window main_window;
	ProgressBar progress_bar;
	StatusBar status_bar;

	using PageMap = std::map<const struct screen_functions *,
				 std::unique_ptr<Page>>;
	PageMap pages;
	PageMap::iterator current_page = pages.begin();

	const struct screen_functions *mode_fn_prev;

	char *buf;
	size_t buf_size;

	char *findbuf;
	GList *find_history = nullptr;

#ifndef NCMPC_MINI
	/**
	 * Non-zero when the welcome message is currently being
	 * displayed.  The associated timer will disable it.
	 */
	guint welcome_source_id;
#endif

	ScreenManager();
	~ScreenManager();

	void Init(struct mpdclient *c);
	void Exit();

	Point GetMainPosition() const {
		return {0, (int)title_bar.GetHeight()};
	}

	PageMap::iterator MakePage(const struct screen_functions &sf);

	void OnResize();

	gcc_pure
	bool IsVisible(const Page &page) const {
		return &page == current_page->second.get();
	}

	void Switch(const struct screen_functions &sf, struct mpdclient *c);
	void Swap(struct mpdclient *c, const struct mpd_song *song);


	void PaintTopWindow();
	void Paint(bool main_dirty);

	void Update(struct mpdclient &c);
	void OnCommand(struct mpdclient *c, command_t cmd);

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient *c, Point p, mmask_t bstate);
#endif

private:
	void NextMode(struct mpdclient &c, int offset);
};

#endif
