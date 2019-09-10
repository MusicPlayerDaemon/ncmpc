/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#include "save_playlist.hxx"
#include "db_completion.hxx"
#include "screen_status.hxx"
#include "config.h" // IWYU pragma: keep
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "Completion.hxx"
#include "screen_utils.hxx"
#include "util/Compiler.h"

#include <mpd/client.h>

#include <stdio.h>

#ifndef NCMPC_MINI

class PlaylistNameCompletion final : public Completion {
	struct mpdclient &c;

public:
	explicit PlaylistNameCompletion(struct mpdclient &_c) noexcept
		:c(_c) {}

protected:
	/* virtual methods from class Completion */
	void Pre(const char *value) noexcept override;
	void Post(const char *value, Range range) noexcept override;
};

void
PlaylistNameCompletion::Pre(gcc_unused const char *value) noexcept
{
	if (empty()) {
		/* create completion list */
		gcmp_list_from_path(&c, "", *this, GCMP_TYPE_PLAYLIST);
	}
}

void
PlaylistNameCompletion::Post(gcc_unused const char *value,
			     Range range) noexcept
{
	if (range.begin() != range.end() &&
	    std::next(range.begin()) != range.end())
		screen_display_completion_list(range);
}

#endif

int
playlist_save(struct mpdclient *c, const char *name,
	      const char *defaultname) noexcept
{
	std::string filename;

	if (name == nullptr) {
#ifdef NCMPC_MINI
		Completion *completion = nullptr;
#else
		/* initialize completion support */
		PlaylistNameCompletion _completion(*c);
		auto *completion = &_completion;
#endif

		/* query the user for a filename */
		filename = screen_readln(_("Save queue as"),
					 defaultname,
					 nullptr,
					 completion);
		if (filename.empty())
			return -1;
	} else
		filename = name;

	/* send save command to mpd */

	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return -1;

	const LocaleToUtf8 filename_utf8(filename.c_str());
	if (!mpd_run_save(connection, filename_utf8.c_str())) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_EXIST &&
		    mpd_connection_clear_error(connection)) {
			char prompt[256];
			snprintf(prompt, sizeof(prompt),
				 _("Replace %s?"), filename.c_str());
			bool replace = screen_get_yesno(prompt, false);
			if (!replace) {
				screen_status_message(_("Aborted"));
				return -1;
			}

			if (!mpd_run_rm(connection, filename_utf8.c_str()) ||
			    !mpd_run_save(connection, filename_utf8.c_str())) {
				c->HandleError();
				return -1;
			}
		} else {
			c->HandleError();
			return -1;
		}
	}

	c->events |= MPD_IDLE_STORED_PLAYLIST;

	/* success */
	screen_status_printf(_("Saved %s"), filename.c_str());
	return 0;
}
