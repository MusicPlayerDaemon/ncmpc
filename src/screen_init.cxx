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
#include "QueuePage.hxx"
#include "config.h"
#include "i18n.h"
#include "Options.hxx"
#include "Styles.hxx"
#include "Event.hxx"

#include <stdlib.h>

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
	buf = new char[buf_size];

	if (!options.hardware_cursor)
		leaveok(main_window.w, true);

	keypad(main_window.w, true);

#ifdef ENABLE_COLORS
	if (options.enable_colors) {
		/* set background attributes */
		wbkgd(stdscr, COLOR_PAIR(Style::LIST));
		wbkgd(main_window.w, COLOR_PAIR(Style::LIST));
	}
#endif
}

ScreenManager::~ScreenManager()
{
	delete[] buf;
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
	delete[] buf;
	buf = new char[buf_size];

	/* resize all screens */
	current_page->second->Resize(main_window.size);

	/* ? - without this the cursor becomes visible with aterm & Eterm */
	curs_set(1);
	curs_set(0);

	Paint(true);
}

void
ScreenManager::Init(struct mpdclient *c)
{
	current_page = MakePage(screen_queue);
	current_page->second->OnOpen(*c);
}
