// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "save_playlist.hxx"
#include "db_completion.hxx"
#include "config.h" // IWYU pragma: keep
#include "i18n.h"
#include "charset.hxx"
#include "Completion.hxx"
#include "screen.hxx"
#include "screen_utils.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "dialogs/YesNoDialog.hxx"
#include "client/mpdclient.hxx"
#include "co/InvokeTask.hxx"

#include <mpd/client.h>

#include <fmt/core.h>

#include <stdio.h>

#ifndef NCMPC_MINI

class PlaylistNameCompletion final : public Completion {
	ScreenManager &screen;
	struct mpdclient &c;

public:
	PlaylistNameCompletion(ScreenManager &_screen,
			       struct mpdclient &_c) noexcept
		:screen(_screen), c(_c) {}

protected:
	/* virtual methods from class Completion */
	void Pre(std::string_view value) noexcept override;
	void Post(std::string_view value, Range range) noexcept override;
};

void
PlaylistNameCompletion::Pre([[maybe_unused]] std::string_view value) noexcept
{
	if (empty()) {
		/* create completion list */
		gcmp_list_from_path(c, "", *this, GCMP_TYPE_PLAYLIST);
	}
}

void
PlaylistNameCompletion::Post(std::string_view value,
			     Range range) noexcept
{
	if (range.begin() != range.end() &&
	    std::next(range.begin()) != range.end())
		screen_display_completion_list(screen.main_window, value, range);
}

#endif

Co::InvokeTask
playlist_save(ScreenManager &screen, struct mpdclient &c,
	      std::string filename) noexcept
{
	if (filename.empty()) {
#ifdef NCMPC_MINI
		Completion *completion = nullptr;
#else
		/* initialize completion support */
		PlaylistNameCompletion _completion{screen, c};
		auto *completion = &_completion;
#endif

		/* query the user for a filename */
		filename = co_await TextInputDialog{
			screen, _("Save queue as"),
			{},
			nullptr,
			completion,
		};

		if (filename.empty())
			co_return;
	}

	/* send save command to mpd */

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		co_return;

	const LocaleToUtf8Z filename_utf8{filename};
	if (!mpd_run_save(connection, filename_utf8.c_str())) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_EXIST &&
		    mpd_connection_clear_error(connection)) {
			char prompt[256];
			snprintf(prompt, sizeof(prompt),
				 _("Replace %s?"), filename.c_str());

			if (co_await YesNoDialog{screen, prompt} != YesNoResult::YES)
				co_return;

			/* obtain a new connection pointer after
			   resuming this coroutine */
			connection = c.GetConnection();
			if (connection == nullptr)
				co_return;

			if (!mpd_run_rm(connection, filename_utf8.c_str()) ||
			    !mpd_run_save(connection, filename_utf8.c_str())) {
				c.HandleError();
				co_return;
			}
		} else {
			c.HandleError();
			co_return;
		}
	}

	c.events |= MPD_IDLE_STORED_PLAYLIST;

	/* success */
	screen.Alert(fmt::format(fmt::runtime(_("Saved {}")), filename));
}
