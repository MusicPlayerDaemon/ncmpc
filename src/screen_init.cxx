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
#include "screen_interface.hxx"
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

void
screen_exit()
{
	if (screen.current_page->close != nullptr)
		screen.current_page->close();

	screen_list_exit();

	string_list_free(screen.find_history);
	g_free(screen.buf);
	g_free(screen.findbuf);

	title_bar_deinit(&screen.title_bar);
	delwin(screen.main_window.w);
	progress_bar_deinit(&screen.progress_bar);
	status_bar_deinit(&screen.status_bar);

#ifndef NCMPC_MINI
	if (screen.welcome_source_id != 0)
		g_source_remove(screen.welcome_source_id);
#endif
}

void
screen_resize(struct mpdclient *c)
{
	const unsigned cols = COLS, rows = LINES;
	if (cols < SCREEN_MIN_COLS || rows < SCREEN_MIN_ROWS) {
		screen_exit();
		fprintf(stderr, "%s\n", _("Error: Screen too small"));
		exit(EXIT_FAILURE);
	}

#ifdef PDCURSES
	resize_term(rows, cols);
#else 
	resizeterm(rows, cols);
#endif

	title_bar_resize(&screen.title_bar, cols);

	/* main window */
	screen.main_window.cols = cols;
	screen.main_window.rows = rows - 4;
	wresize(screen.main_window.w, screen.main_window.rows, cols);

	/* progress window */
	progress_bar_resize(&screen.progress_bar, cols, rows - 2, 0);

	/* status window */
	status_bar_resize(&screen.status_bar, cols, rows - 1, 0);

	screen.buf_size = cols;
	g_free(screen.buf);
	screen.buf = (char *)g_malloc(cols);

	/* resize all screens */
	screen_list_resize(screen.main_window.cols, screen.main_window.rows);

	/* ? - without this the cursor becomes visible with aterm & Eterm */
	curs_set(1);
	curs_set(0);

	screen_paint(c, true);
}

#ifndef NCMPC_MINI
static gboolean
welcome_timer_callback(gpointer data)
{
	auto *c = (struct mpdclient *)data;

	screen.welcome_source_id = 0;

	paint_top_window(c);
	doupdate();

	return false;
}
#endif

void
screen_init(struct mpdclient *c)
{
	const unsigned cols = COLS, rows = LINES;
	if (cols < SCREEN_MIN_COLS || rows < SCREEN_MIN_ROWS) {
		fprintf(stderr, "%s\n", _("Error: Screen too small"));
		exit(EXIT_FAILURE);
	}

	screen.current_page = &screen_queue;
	screen.buf = (char *)g_malloc(cols);
	screen.buf_size = cols;
	screen.findbuf = nullptr;

#ifndef NCMPC_MINI
	if (options.welcome_screen_list)
		screen.welcome_source_id =
			g_timeout_add_seconds(SCREEN_WELCOME_TIME,
					      welcome_timer_callback, c);
#endif

	/* create top window */
	title_bar_init(&screen.title_bar, cols, 0, 0);

	/* create main window */
	window_init(&screen.main_window, rows - 4, cols, 2, 0);

	if (!options.hardware_cursor)
		leaveok(screen.main_window.w, true);

	keypad(screen.main_window.w, true);

	/* create progress window */
	progress_bar_init(&screen.progress_bar, cols, rows - 2, 0);

	/* create status window */
	status_bar_init(&screen.status_bar, cols, rows - 1, 0);

#ifdef ENABLE_COLORS
	if (options.enable_colors) {
		/* set background attributes */
		wbkgd(stdscr, COLOR_PAIR(COLOR_LIST));
		wbkgd(screen.main_window.w,     COLOR_PAIR(COLOR_LIST));
		wbkgd(screen.title_bar.window.w, COLOR_PAIR(COLOR_TITLE));
		wbkgd(screen.progress_bar.window.w,
		      COLOR_PAIR(COLOR_PROGRESSBAR));
		wbkgd(screen.status_bar.window.w, COLOR_PAIR(COLOR_STATUS));
		colors_use(screen.progress_bar.window.w, COLOR_PROGRESSBAR);
	}
#endif

	/* initialize screens */
	screen_list_init(screen.main_window.w,
			 screen.main_window.cols, screen.main_window.rows);

	if (screen.current_page->open != nullptr)
		screen.current_page->open(c);
}
