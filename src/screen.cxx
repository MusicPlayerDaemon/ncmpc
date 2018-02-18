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
#include "screen_status.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "options.hxx"
#include "player_command.hxx"
#include "screen_help.hxx"
#include "screen_queue.hxx"
#include "screen_file.hxx"
#include "screen_artist.hxx"
#include "screen_search.hxx"
#include "screen_song.hxx"
#include "screen_keydef.hxx"
#include "screen_lyrics.hxx"
#include "screen_outputs.hxx"
#include "screen_chat.hxx"

#include <mpd/client.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* screens */

ScreenManager screen;

static const struct screen_functions *mode_fn_prev = &screen_queue;

void
screen_switch(const struct screen_functions *sf, struct mpdclient *c)
{
	assert(sf != nullptr);

	if (sf == screen.current_page)
		return;

	mode_fn_prev = screen.current_page;

	/* close the old mode */
	if (screen.current_page->close != nullptr)
		screen.current_page->close();

	/* get functions for the new mode */
	screen.current_page = sf;

	/* open the new mode */
	if (sf->open != nullptr)
		sf->open(c);

	screen_paint(c, true);
}

void
screen_swap(struct mpdclient *c, const struct mpd_song *song)
{
	if (song != nullptr)
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

	for (i = 0; options.screen_list[i] != nullptr; ++i)
		if (strcmp(options.screen_list[i], name) == 0)
			return i;

	return -1;
}

static void
screen_next_mode(struct mpdclient *c, int offset)
{
	int max = g_strv_length(options.screen_list);

	/* find current screen */
	int current = find_configured_screen(screen_get_name(screen.current_page));
	int next = current + offset;
	if (next<0)
		next = max-1;
	else if (next>=max)
		next = 0;

	const struct screen_functions *sf =
		screen_lookup_name(options.screen_list[next]);
	if (sf != nullptr)
		screen_switch(sf, c);
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
	if ((c->events & MPD_IDLE_OPTIONS) && c->status != nullptr) {
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
	if (screen.current_page->update != nullptr)
		screen.current_page->update(c);

	screen_paint(c, false);
}

void
screen_cmd(struct mpdclient *c, command_t cmd)
{
#ifndef NCMPC_MINI
	if (screen.welcome_source_id != 0) {
		g_source_remove(screen.welcome_source_id);
		screen.welcome_source_id = 0;
	}
#endif

	if (screen.current_page->cmd != nullptr &&
	    screen.current_page->cmd(c, cmd))
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
		screen_paint(c, true);
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
		screen_swap(c, nullptr);
		break;

	default:
		break;
	}
}

#ifdef HAVE_GETMOUSE

static bool
screen_current_page_mouse(struct mpdclient *c, int x, int y, mmask_t bstate)
{
	if (screen.current_page->mouse == nullptr)
		return false;

	y -= screen.title_bar.window.rows;
	return screen.current_page->mouse(c, x, y, bstate);
}

bool
screen_mouse(struct mpdclient *c, int x, int y, mmask_t bstate)
{
	if (screen_current_page_mouse(c, x, y, bstate))
		return true;

	/* if button 2 was pressed switch screen */
	if (bstate & BUTTON2_CLICKED) {
		screen_cmd(c, CMD_SCREEN_NEXT);
		return true;
	}

	return false;
}

#endif
