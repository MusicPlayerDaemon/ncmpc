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

#include "screen.hxx"
#include "Page.hxx"
#include "screen_list.hxx"
#include "screen_queue.hxx"
#include "config.h"
#include "i18n.h"
#include "utils.hxx"
#include "options.hxx"
#include "colors.hxx"

#include <stdlib.h>

#ifndef NCMPC_MINI
/** welcome message time [s] */
static const GTime SCREEN_WELCOME_TIME = 10;
#endif

/* minimum window size */
static const unsigned SCREEN_MIN_COLS = 14;
static const unsigned SCREEN_MIN_ROWS = 5;

ScreenManager::ScreenManager()
	:layout({std::max<unsigned>(COLS, SCREEN_MIN_COLS),
		 std::max<unsigned>(LINES, SCREEN_MIN_ROWS)}),
	 title_bar({layout.title_x, layout.title_y}, layout.size.width),
	 main_window({layout.main_x, layout.main_y}, layout.GetMainSize()),
	 progress_bar({layout.progress_x, layout.GetProgressY()}, layout.size.width),
	 status_bar({layout.status_x, layout.GetStatusY()}, layout.size.width),
	 mode_fn_prev(&screen_queue)
{
	buf_size = layout.size.width;
	buf = (char *)g_malloc(buf_size);
	findbuf = nullptr;

	if (!options.hardware_cursor)
		leaveok(main_window.w, true);

	keypad(main_window.w, true);
}

ScreenManager::~ScreenManager()
{
	string_list_free(find_history);
	g_free(buf);
	g_free(findbuf);

#ifndef NCMPC_MINI
	if (welcome_source_id != 0)
		g_source_remove(welcome_source_id);
#endif
}

void
ScreenManager::Exit()
{
	current_page->second->OnClose();
	current_page = pages.end();
	pages.clear();
}

void
ScreenManager::OnResize()
{
	layout = Layout({std::max<unsigned>(COLS, SCREEN_MIN_COLS),
			 std::max<unsigned>(LINES, SCREEN_MIN_ROWS)});

#ifdef PDCURSES
	resize_term(layout.size.height, layout.size.width);
#else
	resizeterm(layout.size.height, layout.size.width);
#endif

	title_bar.OnResize(layout.size.width);

	/* main window */
	main_window.Resize(layout.GetMainSize());

	/* progress window */
	progress_bar.OnResize({layout.progress_x, layout.GetProgressY()},
			      layout.size.width);

	/* status window */
	status_bar.OnResize({layout.status_x, layout.GetStatusY()},
			    layout.size.width);

	buf_size = layout.size.width;
	g_free(buf);
	buf = (char *)g_malloc(buf_size);

	/* resize all screens */
	for (auto &page : pages)
		page.second->Resize(main_window.size);

	/* ? - without this the cursor becomes visible with aterm & Eterm */
	curs_set(1);
	curs_set(0);

	Paint(true);
}

#ifndef NCMPC_MINI
static gboolean
welcome_timer_callback(gpointer data)
{
	auto &s = *(ScreenManager *)data;

	s.welcome_source_id = 0;

	s.PaintTopWindow();
	doupdate();

	return false;
}
#endif

void
ScreenManager::Init(struct mpdclient *c)
{
#ifndef NCMPC_MINI
	if (options.welcome_screen_list)
		welcome_source_id =
			g_timeout_add_seconds(SCREEN_WELCOME_TIME,
					      welcome_timer_callback, this);
#endif

#ifdef ENABLE_COLORS
	if (options.enable_colors) {
		/* set background attributes */
		wbkgd(stdscr, COLOR_PAIR(COLOR_LIST));
		wbkgd(main_window.w,     COLOR_PAIR(COLOR_LIST));
	}
#endif

	current_page = MakePage(screen_queue);
	current_page->second->OnOpen(*c);
}
