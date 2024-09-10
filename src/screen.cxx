// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen.hxx"
#include "PageMeta.hxx"
#include "screen_list.hxx"
#include "screen_status.hxx"
#include "Command.hxx"
#include "config.h"
#include "i18n.h"
#include "Options.hxx"
#include "DelayedSeek.hxx"
#include "player_command.hxx"
#include "SongPage.hxx"
#include "FileBrowserPage.hxx"
#include "LyricsPage.hxx"
#include "QueuePage.hxx"
#include "page/Page.hxx"
#include "dialogs/ModalDialog.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "ui/Options.hxx"
#include "co/Task.hxx"
#include "client/mpdclient.hxx"
#include "util/ScopeExit.hxx"
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
	GetCurrentPage().OnClose();

	/* get functions for the new mode */
	current_page = page;

	main_dirty = true;

	/* open the new mode */
	auto &p = *page->second;
	p.OnOpen(c);
	p.Resize(layout.GetMainSize());
	p.Update(c);
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

	const bool now_connected = c.IsConnected();
	if (!now_connected && !query_password_busy)
		query_password_task = {};

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
			Alert(mpd_status_get_repeat(c.status) ?
			      _("Repeat mode is on") :
			      _("Repeat mode is off"));

		if (random_enabled != mpd_status_get_random(c.status))
			Alert(mpd_status_get_random(c.status) ?
			      _("Random mode is on") :
			      _("Random mode is off"));

		if (single != mpd_status_get_single(c.status))
			Alert(mpd_status_get_single(c.status) ?
			      /* "single" mode means that MPD will
				 automatically stop after playing one
				 single song */
			      _("Single mode is on") :
			      _("Single mode is off"));

		if (consume != mpd_status_get_consume(c.status))
			Alert(mpd_status_get_consume(c.status) ?
			      /* "consume" mode means that MPD removes
				 each song which has finished
				 playing */
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
	    now_connected)
		Alert(_("Database updated"));
	was_connected = now_connected;
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
	GetCurrentPage().Update(c);

	SchedulePaint();
}

void
ScreenManager::OnCommand(struct mpdclient &c, DelayedSeek &seek, Command cmd)
{
	if (GetCurrentPage().OnCommand(c, cmd))
		return;

	if (handle_player_command(c, seek, cmd))
		return;

	switch(cmd) {
	case Command::TOGGLE_FIND_WRAP:
		ui_options.find_wrap = !ui_options.find_wrap;
		Alert(ui_options.find_wrap ?
		      _("Find mode: Wrapped") :
		      _("Find mode: Normal"));
		return;

	case Command::TOGGLE_AUTOCENTER:
		options.auto_center = !options.auto_center;
		Alert(options.auto_center ?
		      _("Auto center mode: On") :
		      _("Auto center mode: Off"));
		return;

	case Command::SCREEN_UPDATE:
		main_dirty = true;
		SchedulePaint();
		return;

	case Command::SCREEN_PREVIOUS:
		NextMode(c, -1);
		return;

	case Command::SCREEN_NEXT:
		NextMode(c, 1);
		return;

	case Command::SCREEN_SWAP:
		Swap(c, GetCurrentPage().GetSelectedSong());
		return;

	case Command::LOCATE:
		if (!IsVisible(screen_browse)) {
			if (const auto *song = GetCurrentPage().GetSelectedSong()) {
				screen_file_goto_song(*this, c, *song);
				return;
			}
		}

		break;

#ifdef ENABLE_SONG_SCREEN
	case Command::SCREEN_SONG:
		if (!IsVisible(screen_song)) {
			if (const auto *song = GetCurrentPage().GetSelectedSong()) {
				screen_song_switch(*this, c, *song);
				return;
			}
		}

		break;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	case Command::SCREEN_LYRICS:
		if (!IsVisible(screen_lyrics)) {
			if (const auto *song = GetCurrentPage().GetSelectedSong()) {
				screen_lyrics_switch(*this, c, *song,
						     IsVisible(screen_queue) && song == c.GetPlayingSong());
				return;
			}
		}

		break;
#endif

	default:
		break;
	}

	if (const auto *new_page = PageByCommand(cmd)) {
		Switch(*new_page, c);
		return;
	}
}

#ifdef HAVE_GETMOUSE

bool
ScreenManager::OnMouse(struct mpdclient &c, DelayedSeek &seek,
		       Point p, mmask_t bstate)
{
	if (GetCurrentPage().OnMouse(c, p - GetMainPosition(), bstate))
		return true;

	/* if button 2 was pressed switch screen */
	if (bstate & BUTTON2_CLICKED) {
		OnCommand(c, seek, Command::SCREEN_NEXT);
		return true;
	}

	return false;
}

#endif

static Co::Task<std::string>
EnterPassword(ScreenManager &screen, struct mpdclient &c)
{
	assert(c.IsConnected());
	assert(!c.authenticating);

	/* set the "authenticating" flag to exclude others from using
	   the connection meanwhile */
	c.authenticating = true;
	AtScopeExit(&c) {
		/* make sure that the flag gets reverted even if the
		   coroutine is canceled */
		c.authenticating = false;
	};

	co_return co_await TextInputDialog{
		screen, _("Password"), {},
		nullptr, nullptr, true,
	};
}

inline Co::InvokeTask
ScreenManager::_QueryPassword(struct mpdclient &c)
{
	assert(c.IsConnected());

	if (c.authenticating)
		// shouldn't happen
		co_return;

	const auto password = co_await EnterPassword(*this, c);
	if (password.empty())
		co_return;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		co_return;

	assert(!query_password_busy);
	query_password_busy = true;
	AtScopeExit(this) {
		assert(query_password_busy);
		query_password_busy = false;
	};

	mpd_send_password(connection, password.c_str());

	if (!mpd_response_finish(connection)) {
		c.HandleAuthError();
		co_return;
	}

	c.AuthenticationFinished();
}

void
ScreenManager::QueryPassword(struct mpdclient &c) noexcept
{
	if (query_password_task)
		// already running
		return;

	query_password_task = _QueryPassword(c);
	query_password_task.Start(BIND_THIS_METHOD(OnCoComplete));
}

inline void
ScreenManager::OnCoComplete(std::exception_ptr error) noexcept
{
	if (error)
		screen_status_error(std::move(error));
}

void
ScreenManager::ShowModalDialog(ModalDialog &m) noexcept
{
	assert(&m != modal);

	status_bar.ClearMessage();

	CancelModalDialog();
	assert(modal == nullptr);

	modal = &m;

	m.OnResize(status_bar.GetWindow(), layout.GetStatusSize());

	SchedulePaint();
}

void
ScreenManager::HideModalDialog([[maybe_unused]] ModalDialog &m) noexcept
{
	assert(&m == modal);
	modal = nullptr;

	m.OnLeave(status_bar.GetWindow());

	SchedulePaint();
}

void
ScreenManager::CancelModalDialog(ModalDialog &m) noexcept
{
	assert(&m == modal);

	modal = nullptr;
	m.OnLeave(status_bar.GetWindow());
	m.InvokeCancel();

	SchedulePaint();
}

bool
ScreenManager::CancelModalDialog() noexcept
{
	if (modal == nullptr)
		return false;

	CancelModalDialog(*modal);
	return true;
}

bool
ScreenManager::OnModalDialogKey(int key)
{
	if (modal == nullptr)
		return false;

	modal->OnKey(status_bar.GetWindow(), key);
	return true;
}

void
ScreenManager::SchedulePaint(Page &page) noexcept
{
	if (!IsVisible(page))
		return;

	main_dirty = true;
	SchedulePaint();
}

void
ScreenManager::Alert(std::string message) noexcept
{
	status_bar.SetMessage(std::move(message));
}
