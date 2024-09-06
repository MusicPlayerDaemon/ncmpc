// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen.hxx"
#include "PageMeta.hxx"
#include "screen_list.hxx"
#include "screen_status.hxx"
#include "Page.hxx"
#include "Command.hxx"
#include "config.h"
#include "i18n.h"
#include "mpdclient.hxx"
#include "Options.hxx"
#include "DelayedSeek.hxx"
#include "player_command.hxx"
#include "SongPage.hxx"
#include "LyricsPage.hxx"
#include "ui/Options.hxx"
#include "util/StringAPI.hxx"

#include <mpd/client.h>

ScreenManager::PageMap::iterator
ScreenManager::MakePage(const PageMeta &sf) noexcept
{
	auto i = pages.find(&sf);
	if (i != pages.end())
		return i;

	auto j = pages.emplace(&sf,
			       sf.init(*this, main_window,
				       layout.GetMainSize()));
	assert(j.second);
	return j.first;
}

void
ScreenManager::Switch(const PageMeta &sf, struct mpdclient &c) noexcept
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
	auto &p = *page->second;
	p.OnOpen(c);
	p.Resize(layout.GetMainSize());
	p.Update(c);
	p.SetDirty();
}

void
ScreenManager::Swap(struct mpdclient &c, const struct mpd_song *song) noexcept
{
	if (song != nullptr)
	{
		if (false)
			{ /* just a hack to make the ifdefs less ugly */ }
#ifdef ENABLE_SONG_SCREEN
		if (mode_fn_prev == &screen_song)
			screen_song_switch(*this, c, *song);
#endif
#ifdef ENABLE_LYRICS_SCREEN
		else if (mode_fn_prev == &screen_lyrics)
			screen_lyrics_switch(*this, c, *song, true);
#endif
		else
			Switch(*mode_fn_prev, c);
	}
	else
		Switch(*mode_fn_prev, c);
}

[[gnu::pure]]
static int
find_configured_screen(const char *name) noexcept
{
	unsigned i;

	for (i = 0; i < options.screen_list.size(); ++i)
		if (StringIsEqual(options.screen_list[i].c_str(), name))
			return i;

	return -1;
}

void
ScreenManager::NextMode(struct mpdclient &c, int offset) noexcept
{
	int max = options.screen_list.size();

	/* find current screen */
	int current = find_configured_screen(current_page->first->name);
	int next = current + offset;
	if (next<0)
		next = max-1;
	else if (next>=max)
		next = 0;

	const PageMeta *sf =
		screen_lookup_name(options.screen_list[next].c_str());
	if (sf != nullptr)
		Switch(*sf, c);
}

void
ScreenManager::Update(struct mpdclient &c, const DelayedSeek &seek) noexcept
{
	const unsigned events = c.events;

#ifndef NCMPC_MINI
	static bool was_connected;
	static bool initialized = false;
	static bool repeat;
	static bool random_enabled;
	static bool single;
	static bool consume;
	static unsigned crossfade;

	/* print a message if mpd status has changed */
	if ((events & MPD_IDLE_OPTIONS) && c.status != nullptr) {
		if (!initialized) {
			repeat = mpd_status_get_repeat(c.status);
			random_enabled = mpd_status_get_random(c.status);
			single = mpd_status_get_single(c.status);
			consume = mpd_status_get_consume(c.status);
			crossfade = mpd_status_get_crossfade(c.status);
			initialized = true;
		}

		if (repeat != mpd_status_get_repeat(c.status))
			screen_status_message(mpd_status_get_repeat(c.status) ?
					      _("Repeat mode is on") :
					      _("Repeat mode is off"));

		if (random_enabled != mpd_status_get_random(c.status))
			screen_status_message(mpd_status_get_random(c.status) ?
					      _("Random mode is on") :
					      _("Random mode is off"));

		if (single != mpd_status_get_single(c.status))
			screen_status_message(mpd_status_get_single(c.status) ?
					      /* "single" mode means
						 that MPD will
						 automatically stop
						 after playing one
						 single song */
					      _("Single mode is on") :
					      _("Single mode is off"));

		if (consume != mpd_status_get_consume(c.status))
			screen_status_message(mpd_status_get_consume(c.status) ?
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

	if ((events & MPD_IDLE_DATABASE) != 0 && was_connected &&
	    c.IsConnected())
		screen_status_message(_("Database updated"));
	was_connected = c.IsConnected();
#endif

	title_bar.Update(c.status);

	unsigned elapsed;
	if (c.status == nullptr)
		elapsed = 0;
	else if (seek.IsSeeking(mpd_status_get_song_id(c.status)))
		elapsed = seek.GetTime();
	else
		elapsed = mpd_status_get_elapsed_time(c.status);

	unsigned duration = c.playing_or_paused
		? mpd_status_get_total_time(c.status)
		: 0;

	progress_bar.Set(elapsed, duration);

	status_bar.Update(c.status, c.GetCurrentSong(), seek);

	for (auto &i : pages)
		i.second->AddPendingEvents(events);

	/* update the main window */
	current_page->second->Update(c);

	SchedulePaint();
}

void
ScreenManager::OnCommand(struct mpdclient &c, DelayedSeek &seek, Command cmd)
{
	if (current_page->second->OnCommand(c, cmd))
		return;

	if (handle_player_command(c, seek, cmd))
		return;

	const auto *new_page = PageByCommand(cmd);
	if (new_page != nullptr) {
		Switch(*new_page, c);
		return;
	}

	switch(cmd) {
	case Command::TOGGLE_FIND_WRAP:
		ui_options.find_wrap = !ui_options.find_wrap;
		screen_status_message(ui_options.find_wrap ?
				      _("Find mode: Wrapped") :
				      _("Find mode: Normal"));
		break;
	case Command::TOGGLE_AUTOCENTER:
		options.auto_center = !options.auto_center;
		screen_status_message(options.auto_center ?
				      _("Auto center mode: On") :
				      _("Auto center mode: Off"));
		break;
	case Command::SCREEN_UPDATE:
		current_page->second->SetDirty();
		break;
	case Command::SCREEN_PREVIOUS:
		NextMode(c, -1);
		break;
	case Command::SCREEN_NEXT:
		NextMode(c, 1);
		break;
	case Command::SCREEN_SWAP:
		Swap(c, nullptr);
		break;

	default:
		break;
	}
}

#ifdef HAVE_GETMOUSE

bool
ScreenManager::OnMouse(struct mpdclient &c, DelayedSeek &seek,
		       Point p, mmask_t bstate)
{
	if (current_page->second->OnMouse(c, p - GetMainPosition(),
					  bstate))
		return true;

	/* if button 2 was pressed switch screen */
	if (bstate & BUTTON2_CLICKED) {
		OnCommand(c, seek, Command::SCREEN_NEXT);
		return true;
	}

	return false;
}

#endif
