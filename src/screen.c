/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "screen.h"
#include "screen_interface.h"
#include "screen_list.h"
#include "screen_utils.h"
#include "screen_status.h"
#include "config.h"
#include "i18n.h"
#include "charset.h"
#include "mpdclient.h"
#include "utils.h"
#include "options.h"
#include "colors.h"
#include "player_command.h"
#include "screen_help.h"
#include "screen_queue.h"
#include "screen_file.h"
#include "screen_artist.h"
#include "screen_search.h"
#include "screen_song.h"
#include "screen_keydef.h"
#include "screen_lyrics.h"
#include "screen_outputs.h"
#include "screen_chat.h"

#include <mpd/client.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#ifndef NCMPC_MINI
/** welcome message time [s] */
static const GTime SCREEN_WELCOME_TIME = 10;
#endif

/* minimum window size */
static const int SCREEN_MIN_COLS = 14;
static const int SCREEN_MIN_ROWS = 5;

/* screens */

struct screen screen;
static const struct screen_functions *mode_fn = &screen_queue;
static const struct screen_functions *mode_fn_prev = &screen_queue;

gboolean
screen_is_visible(const struct screen_functions *sf)
{
	return sf == mode_fn;
}

void
screen_switch(const struct screen_functions *sf, struct mpdclient *c)
{
	assert(sf != NULL);

	if (sf == mode_fn)
		return;

	mode_fn_prev = mode_fn;

	/* close the old mode */
	if (mode_fn->close != NULL)
		mode_fn->close();

	/* get functions for the new mode */
	mode_fn = sf;

	/* open the new mode */
	if (mode_fn->open != NULL)
		mode_fn->open(c);

	screen_paint(c);
}

void
screen_swap(struct mpdclient *c, const struct mpd_song *song)
{
	if (song != NULL)
	{
		if (false)
			{ /* just a hack to make the ifdefs less ugly */ }
#ifdef ENABLE_SONG_SCREEN
		if (mode_fn_prev == &screen_song)
			screen_song_switch(c, song);
#endif
#ifdef ENABLE_LYRICS_SCREEN
		else if (mode_fn_prev == &screen_lyrics)
			screen_lyrics_switch(c, song, true);
#endif
		else
			screen_switch(mode_fn_prev, c);
	}
	else
		screen_switch(mode_fn_prev, c);
}

static int
find_configured_screen(const char *name)
{
	unsigned i;

	for (i = 0; options.screen_list[i] != NULL; ++i)
		if (strcmp(options.screen_list[i], name) == 0)
			return i;

	return -1;
}

static void
screen_next_mode(struct mpdclient *c, int offset)
{
	int max = g_strv_length(options.screen_list);

	/* find current screen */
	int current = find_configured_screen(screen_get_name(mode_fn));
	int next = current + offset;
	if (next<0)
		next = max-1;
	else if (next>=max)
		next = 0;

	const struct screen_functions *sf =
		screen_lookup_name(options.screen_list[next]);
	if (sf != NULL)
		screen_switch(sf, c);
}

static void
paint_top_window(const char *header, const struct mpdclient *c)
{
	title_bar_paint(&screen.title_bar, header, c->status);
}

static void
update_progress_window(struct mpdclient *c, bool repaint)
{
	unsigned elapsed;
	if (c->status == NULL)
		elapsed = 0;
	else if (seek_id >= 0 && seek_id == mpd_status_get_song_id(c->status))
		elapsed = seek_target_time;
	else
		elapsed = mpd_status_get_elapsed_time(c->status);

	unsigned duration = mpdclient_is_playing(c)
		? mpd_status_get_total_time(c->status)
		: 0;

	if (progress_bar_set(&screen.progress_bar, elapsed, duration) ||
	    repaint)
		progress_bar_paint(&screen.progress_bar);
}

void
screen_exit(void)
{
	if (mode_fn->close != NULL)
		mode_fn->close();

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
	if (COLS<SCREEN_MIN_COLS || LINES<SCREEN_MIN_ROWS) {
		screen_exit();
		fprintf(stderr, "%s\n", _("Error: Screen too small"));
		exit(EXIT_FAILURE);
	}
#ifdef PDCURSES
	resize_term(LINES, COLS);
#else 
	resizeterm(LINES, COLS);
#endif

	screen.cols = COLS;
	screen.rows = LINES;

	title_bar_resize(&screen.title_bar, screen.cols);

	/* main window */
	screen.main_window.cols = screen.cols;
	screen.main_window.rows = screen.rows-4;
	wresize(screen.main_window.w, screen.main_window.rows, screen.cols);
	wclear(screen.main_window.w);

	/* progress window */
	progress_bar_resize(&screen.progress_bar, screen.cols,
			    screen.rows - 2, 0);
	progress_bar_paint(&screen.progress_bar);

	/* status window */
	status_bar_resize(&screen.status_bar, screen.cols, screen.rows - 1, 0);
	status_bar_paint(&screen.status_bar, c->status, c->song);

	screen.buf_size = screen.cols;
	g_free(screen.buf);
	screen.buf = g_malloc(screen.cols);

	/* resize all screens */
	screen_list_resize(screen.main_window.cols, screen.main_window.rows);

	/* ? - without this the cursor becomes visible with aterm & Eterm */
	curs_set(1);
	curs_set(0);

	screen_paint(c);
}

#ifndef NCMPC_MINI
static gboolean
welcome_timer_callback(gpointer data)
{
	struct mpdclient *c = data;

#ifndef NCMPC_MINI
	screen.welcome_source_id = 0;
#endif

	paint_top_window(mode_fn->get_title != NULL
			 ? mode_fn->get_title(screen.buf, screen.buf_size)
			 : "",
			 c);
	doupdate();

	return false;
}
#endif

void
screen_init(struct mpdclient *c)
{
	if (COLS < SCREEN_MIN_COLS || LINES < SCREEN_MIN_ROWS) {
		fprintf(stderr, "%s\n", _("Error: Screen too small"));
		exit(EXIT_FAILURE);
	}

	screen.cols = COLS;
	screen.rows = LINES;

	screen.buf  = g_malloc(screen.cols);
	screen.buf_size = screen.cols;
	screen.findbuf = NULL;

#ifndef NCMPC_MINI
	if (options.welcome_screen_list)
		screen.welcome_source_id =
			g_timeout_add(SCREEN_WELCOME_TIME * 1000,
				      welcome_timer_callback, c);
#endif

	/* create top window */
	title_bar_init(&screen.title_bar, screen.cols, 0, 0);

	/* create main window */
	window_init(&screen.main_window, screen.rows - 4, screen.cols, 2, 0);

	if (!options.hardware_cursor)
		leaveok(screen.main_window.w, TRUE);

	keypad(screen.main_window.w, TRUE);

	/* create progress window */
	progress_bar_init(&screen.progress_bar, screen.cols,
			  screen.rows - 2, 0);
	progress_bar_paint(&screen.progress_bar);

	/* create status window */
	status_bar_init(&screen.status_bar, screen.cols, screen.rows - 1, 0);
	status_bar_paint(&screen.status_bar, c->status, c->song);

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

	if (mode_fn->open != NULL)
		mode_fn->open(c);
}

static void
screen_refresh(struct mpdclient *c, bool main_dirty)
{
	/* update title/header window */
	const char *title =
#ifndef NCMPC_MINI
		screen.welcome_source_id == 0 &&
#endif
		mode_fn->get_title != NULL
		? mode_fn->get_title(screen.buf, screen.buf_size)
		: "";
	assert(title != NULL);
	paint_top_window(title, c);

	/* paint the bottom window */

	update_progress_window(c, true);
	status_bar_paint(&screen.status_bar, c->status, c->song);

	/* paint the main window */

	if (main_dirty) {
		wclear(screen.main_window.w);
		if (mode_fn->paint != NULL)
			mode_fn->paint();
	}

	/* move the cursor to the origin */

	if (!options.hardware_cursor)
		wmove(screen.main_window.w, 0, 0);

	wnoutrefresh(screen.main_window.w);

	/* tell curses to update */
	doupdate();
}

void
screen_paint(struct mpdclient *c)
{
	screen_refresh(c, true);
}

void
screen_update(struct mpdclient *c)
{
#ifndef NCMPC_MINI
	static bool was_connected;
	static bool initialized = false;
	static bool repeat;
	static bool random_enabled;
	static bool single;
	static bool consume;
	static unsigned crossfade;

	/* print a message if mpd status has changed */
	if ((c->events & MPD_IDLE_OPTIONS) && c->status != NULL) {
		if (!initialized) {
			repeat = mpd_status_get_repeat(c->status);
			random_enabled = mpd_status_get_random(c->status);
			single = mpd_status_get_single(c->status);
			consume = mpd_status_get_consume(c->status);
			crossfade = mpd_status_get_crossfade(c->status);
			initialized = true;
		}

		if (repeat != mpd_status_get_repeat(c->status))
			screen_status_printf(mpd_status_get_repeat(c->status) ?
					     _("Repeat mode is on") :
					     _("Repeat mode is off"));

		if (random_enabled != mpd_status_get_random(c->status))
			screen_status_printf(mpd_status_get_random(c->status) ?
					     _("Random mode is on") :
					     _("Random mode is off"));

		if (single != mpd_status_get_single(c->status))
			screen_status_printf(mpd_status_get_single(c->status) ?
					     /* "single" mode means
						that MPD will
						automatically stop
						after playing one
						single song */
					     _("Single mode is on") :
					     _("Single mode is off"));

		if (consume != mpd_status_get_consume(c->status))
			screen_status_printf(mpd_status_get_consume(c->status) ?
					     /* "consume" mode means
						that MPD removes each
						song which has
						finished playing */
					     _("Consume mode is on") :
					     _("Consume mode is off"));

		if (crossfade != mpd_status_get_crossfade(c->status))
			screen_status_printf(_("Crossfade %d seconds"),
					     mpd_status_get_crossfade(c->status));

		repeat = mpd_status_get_repeat(c->status);
		random_enabled = mpd_status_get_random(c->status);
		single = mpd_status_get_single(c->status);
		consume = mpd_status_get_consume(c->status);
		crossfade = mpd_status_get_crossfade(c->status);
	}

	if ((c->events & MPD_IDLE_DATABASE) != 0 && was_connected &&
	    mpdclient_is_connected(c))
		screen_status_printf(_("Database updated"));
	was_connected = mpdclient_is_connected(c);
#endif

	/* update the main window */
	if (mode_fn->update != NULL)
		mode_fn->update(c);

	screen_refresh(c, false);
}

#ifdef HAVE_GETMOUSE
int
screen_get_mouse_event(struct mpdclient *c, unsigned long *bstate, int *row)
{
	MEVENT event;

	/* retrieve the mouse event from curses */
#ifdef PDCURSES
	nc_getmouse(&event);
#else
	getmouse(&event);
#endif
	/* calculate the selected row in the list window */
	*row = event.y - screen.title_bar.window.rows;
	/* copy button state bits */
	*bstate = event.bstate;
	/* if button 2 was pressed switch screen */
	if (event.bstate & BUTTON2_CLICKED) {
		screen_cmd(c, CMD_SCREEN_NEXT);
		return 1;
	}

	return 0;
}
#endif

void
screen_cmd(struct mpdclient *c, command_t cmd)
{
#ifndef NCMPC_MINI
	if (screen.welcome_source_id != 0) {
		g_source_remove(screen.welcome_source_id);
		screen.welcome_source_id = 0;
	}
#endif

	if (mode_fn->cmd != NULL && mode_fn->cmd(c, cmd))
		return;

	if (handle_player_command(c, cmd))
		return;

	switch(cmd) {
	case CMD_TOGGLE_FIND_WRAP:
		options.find_wrap = !options.find_wrap;
		screen_status_printf(options.find_wrap ?
				     _("Find mode: Wrapped") :
				     _("Find mode: Normal"));
		break;
	case CMD_TOGGLE_AUTOCENTER:
		options.auto_center = !options.auto_center;
		screen_status_printf(options.auto_center ?
				     _("Auto center mode: On") :
				     _("Auto center mode: Off"));
		break;
	case CMD_SCREEN_UPDATE:
		screen_paint(c);
		break;
	case CMD_SCREEN_PREVIOUS:
		screen_next_mode(c, -1);
		break;
	case CMD_SCREEN_NEXT:
		screen_next_mode(c, 1);
		break;
	case CMD_SCREEN_PLAY:
		screen_switch(&screen_queue, c);
		break;
	case CMD_SCREEN_FILE:
		screen_switch(&screen_browse, c);
		break;
#ifdef ENABLE_HELP_SCREEN
	case CMD_SCREEN_HELP:
		screen_switch(&screen_help, c);
		break;
#endif
#ifdef ENABLE_SEARCH_SCREEN
	case CMD_SCREEN_SEARCH:
		screen_switch(&screen_search, c);
		break;
#endif
#ifdef ENABLE_ARTIST_SCREEN
	case CMD_SCREEN_ARTIST:
		screen_switch(&screen_artist, c);
		break;
#endif
#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		screen_switch(&screen_song, c);
		break;
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	case CMD_SCREEN_KEYDEF:
		screen_switch(&screen_keydef, c);
		break;
#endif
#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		screen_switch(&screen_lyrics, c);
		break;
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	case CMD_SCREEN_OUTPUTS:
		screen_switch(&screen_outputs, c);
		break;
#endif
#ifdef ENABLE_CHAT_SCREEN
	case CMD_SCREEN_CHAT:
		screen_switch(&screen_chat, c);
		break;
#endif
	case CMD_SCREEN_SWAP:
		screen_swap(c, NULL);
		break;

	default:
		break;
	}
}
