// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "save_playlist.hxx"
#include "db_completion.hxx"
#include "screen_status.hxx"
#include "config.h" // IWYU pragma: keep
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "Completion.hxx"
#include "screen_utils.hxx"

#include <mpd/client.h>

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
PlaylistNameCompletion::Post([[maybe_unused]] std::string_view value,
			     Range range) noexcept
{
	if (range.begin() != range.end() &&
	    std::next(range.begin()) != range.end())
		screen_display_completion_list(screen, range);
}

#endif

int
playlist_save(ScreenManager &screen, struct mpdclient &c,
	      const char *name) noexcept
{
	std::string filename;

	if (name == nullptr) {
#ifdef NCMPC_MINI
		Completion *completion = nullptr;
#else
		/* initialize completion support */
		PlaylistNameCompletion _completion{screen, c};
		auto *completion = &_completion;
#endif

		/* query the user for a filename */
		filename = screen_readln(screen, _("Save queue as"),
					 nullptr,
					 nullptr,
					 completion);
		if (filename.empty())
			return -1;
	} else
		filename = name;

	/* send save command to mpd */

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return -1;

	const LocaleToUtf8Z filename_utf8{filename};
	if (!mpd_run_save(connection, filename_utf8.c_str())) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_EXIST &&
		    mpd_connection_clear_error(connection)) {
			char prompt[256];
			snprintf(prompt, sizeof(prompt),
				 _("Replace %s?"), filename.c_str());
			bool replace = screen_get_yesno(screen, prompt, false);
			if (!replace)
				return -1;

			if (!mpd_run_rm(connection, filename_utf8.c_str()) ||
			    !mpd_run_save(connection, filename_utf8.c_str())) {
				c.HandleError();
				return -1;
			}
		} else {
			c.HandleError();
			return -1;
		}
	}

	c.events |= MPD_IDLE_STORED_PLAYLIST;

	/* success */
	screen_status_printf(_("Saved %s"), filename.c_str());
	return 0;
}
