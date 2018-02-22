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
#include "Page.hxx"
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

static const struct screen_functions *mode_fn_prev = &screen_queue;

ScreenManager::PageMap::iterator
ScreenManager::MakePage(const struct screen_functions &sf)
{
	auto i = pages.find(&sf);
	if (i != pages.end())
		return i;

	auto j = pages.emplace(&sf,
			       sf.init(*this, main_window.w,
				       main_window.cols, main_window.rows));
	assert(j.second);
	return j.first;
}

void
ScreenManager::Switch(const struct screen_functions &sf, struct mpdclient *c)
{
	if (&sf == current_page->first)
		return;

	auto page = MakePage(sf);

	mode_fn_prev = &*current_page->first;

	/* close the old mode */
	current_page->second->OnClose();

	/* get functions for the new mode */
	current_page = page;

	/* open the new mode */
	page->second->OnOpen(*c);

	Paint(true);
}

void
ScreenManager::Swap(struct mpdclient *c, const struct mpd_song *song)
{
	if (song != nullptr)
	{
		if (false)
			{ /* just a hack to make the ifdefs less ugly */ }
#ifdef ENABLE_SONG_SCREEN
		if (mode_fn_prev == &screen_song)
			screen_song_switch(*this, *c, *song);
#endif
#ifdef ENABLE_LYRICS_SCREEN
		else if (mode_fn_prev == &screen_lyrics)
			screen_lyrics_switch(*this, *c, *song, true);
#endif
		else
			Switch(*mode_fn_prev, c);
	}
	else
		Switch(*mode_fn_prev, c);
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

void
ScreenManager::NextMode(struct mpdclient &c, int offset)
{
	int max = g_strv_length(options.screen_list);

	/* find current screen */
	int current = find_configured_screen(current_page->first->name);
	int next = current + offset;
	if (next<0)
		next = max-1;
	else if (next>=max)
		next = 0;

	const struct screen_functions *sf =
		screen_lookup_name(options.screen_list[next]);
	if (sf != nullptr)
		Switch(*sf, &c);
}

void
ScreenManager::Update(struct mpdclient &c)
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
	if ((c.events & MPD_IDLE_OPTIONS) && c.status != nullptr) {
		if (!initialized) {
			repeat = mpd_status_get_repeat(c.status);
			random_enabled = mpd_status_get_random(c.status);
			single = mpd_status_get_single(c.status);
			consume = mpd_status_get_consume(c.status);
			crossfade = mpd_status_get_crossfade(c.status);
			initialized = true;
		}

		if (repeat != mpd_status_get_repeat(c.status))
			screen_status_printf(mpd_status_get_repeat(c.status) ?
					     _("Repeat mode is on") :
					     _("Repeat mode is off"));

		if (random_enabled != mpd_status_get_random(c.status))
			screen_status_printf(mpd_status_get_random(c.status) ?
					     _("Random mode is on") :
					     _("Random mode is off"));

		if (single != mpd_status_get_single(c.status))
			screen_status_printf(mpd_status_get_single(c.status) ?
					     /* "single" mode means
						that MPD will
						automatically stop
						after playing one
						single song */
					     _("Single mode is on") :
					     _("Single mode is off"));

		if (consume != mpd_status_get_consume(c.status))
			screen_status_printf(mpd_status_get_consume(c.status) ?
					     /* "consume" mode means
						that MPD removes each
						song which has
						finished playing */
					     _("Consume mode is on") :
					     _("Consume mode is off"));

		if (crossfade != mpd_status_get_crossfade(c.status))
			screen_status_printf(_("Crossfade %d seconds"),
					     mpd_status_get_crossfade(c.status));

		repeat = mpd_status_get_repeat(c.status);
		random_enabled = mpd_status_get_random(c.status);
		single = mpd_status_get_single(c.status);
		consume = mpd_status_get_consume(c.status);
		crossfade = mpd_status_get_crossfade(c.status);
	}

	if ((c.events & MPD_IDLE_DATABASE) != 0 && was_connected &&
	    c.IsConnected())
		screen_status_printf(_("Database updated"));
	was_connected = c.IsConnected();
#endif

	title_bar.Update(c.status);

	unsigned elapsed;
	if (c.status == nullptr)
		elapsed = 0;
	else if (seek_id >= 0 && seek_id == mpd_status_get_song_id(c.status))
		elapsed = seek_target_time;
	else
		elapsed = mpd_status_get_elapsed_time(c.status);

	unsigned duration = mpdclient_is_playing(&c)
		? mpd_status_get_total_time(c.status)
		: 0;

	progress_bar.Set(elapsed, duration);

	status_bar.Update(c.status, c.song);

	/* update the main window */
	current_page->second->Update(c);

	Paint(current_page->second->IsDirty());
}

void
ScreenManager::OnCommand(struct mpdclient *c, command_t cmd)
{
#ifndef NCMPC_MINI
	if (welcome_source_id != 0) {
		g_source_remove(welcome_source_id);
		welcome_source_id = 0;
	}
#endif

	if (current_page->second->OnCommand(*c, cmd))
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
		Paint(true);
		break;
	case CMD_SCREEN_PREVIOUS:
		NextMode(*c, -1);
		break;
	case CMD_SCREEN_NEXT:
		NextMode(*c, 1);
		break;
	case CMD_SCREEN_PLAY:
		Switch(screen_queue, c);
		break;
	case CMD_SCREEN_FILE:
		Switch(screen_browse, c);
		break;
#ifdef ENABLE_HELP_SCREEN
	case CMD_SCREEN_HELP:
		Switch(screen_help, c);
		break;
#endif
#ifdef ENABLE_SEARCH_SCREEN
	case CMD_SCREEN_SEARCH:
		Switch(screen_search, c);
		break;
#endif
#ifdef ENABLE_ARTIST_SCREEN
	case CMD_SCREEN_ARTIST:
		Switch(screen_artist, c);
		break;
#endif
#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		Switch(screen_song, c);
		break;
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	case CMD_SCREEN_KEYDEF:
		Switch(screen_keydef, c);
		break;
#endif
#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		Switch(screen_lyrics, c);
		break;
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	case CMD_SCREEN_OUTPUTS:
		Switch(screen_outputs, c);
		break;
#endif
#ifdef ENABLE_CHAT_SCREEN
	case CMD_SCREEN_CHAT:
		Switch(screen_chat, c);
		break;
#endif
	case CMD_SCREEN_SWAP:
		Swap(c, nullptr);
		break;

	default:
		break;
	}
}

#ifdef HAVE_GETMOUSE

bool
ScreenManager::OnMouse(struct mpdclient *c, int x, int y, mmask_t bstate)
{
	if (current_page->second->OnMouse(*c, x,
					  y - title_bar.GetHeight(),
					  bstate))
		return true;

	/* if button 2 was pressed switch screen */
	if (bstate & BUTTON2_CLICKED) {
		OnCommand(c, CMD_SCREEN_NEXT);
		return true;
	}

	return false;
}

#endif
