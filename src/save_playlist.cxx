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

#include "save_playlist.hxx"
#include "db_completion.hxx"
#include "screen_status.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "utils.hxx"
#include "wreadln.hxx"
#include "Completion.hxx"
#include "screen_utils.hxx"
#include "Compiler.h"

#include <mpd/client.h>

#include <glib.h>

#include <string.h>

#ifndef NCMPC_MINI

class PlaylistNameCompletion final : public Completion {
	struct mpdclient &c;
	GList *list = nullptr;

public:
	explicit PlaylistNameCompletion(struct mpdclient &_c)
		:c(_c) {}

	~PlaylistNameCompletion() {
		string_list_free(list);
	}

protected:
	/* virtual methods from class Completion */
	void Pre(const char *value) override;
	void Post(const char *value, GList *items) override;
};

void
PlaylistNameCompletion::Pre(gcc_unused const char *value)
{
	if (list == nullptr) {
		/* create completion list */
		list = gcmp_list_from_path(&c, "", nullptr, GCMP_TYPE_PLAYLIST);
		g_completion_add_items(gcmp, list);
	}
}

void
PlaylistNameCompletion::Post(gcc_unused const char *value, GList *items)
{
	if (g_list_length(items) >= 1)
		screen_display_completion_list(items);
}

#endif

int
playlist_save(struct mpdclient *c, char *name, char *defaultname)
{
	struct mpd_connection *connection;
	gchar *filename;

#ifdef NCMPC_MINI
	(void)defaultname;
#endif

#ifndef NCMPC_MINI
	if (name == nullptr) {
		/* initialize completion support */
		PlaylistNameCompletion completion(*c);

		/* query the user for a filename */
		auto result = screen_readln(_("Save queue as"),
					    defaultname,
					    nullptr,
					    &completion);
		if (result.empty())
			return -1;

		filename = g_strdup(result.c_str());
		filename = g_strstrip(filename);
	} else
#endif
		filename=g_strdup(name);

	/* send save command to mpd */

	connection = mpdclient_get_connection(c);
	if (connection == nullptr) {
		g_free(filename);
		return -1;
	}

	const LocaleToUtf8 filename_utf8(filename);
	if (!mpd_run_save(connection, filename_utf8.c_str())) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_EXIST &&
		    mpd_connection_clear_error(connection)) {
			char *buf = g_strdup_printf(_("Replace %s?"), filename);
			bool replace = screen_get_yesno(buf, false);
			g_free(buf);

			if (!replace) {
				g_free(filename);
				screen_status_printf(_("Aborted"));
				return -1;
			}

			if (!mpd_run_rm(connection, filename_utf8.c_str()) ||
			    !mpd_run_save(connection, filename_utf8.c_str())) {
				mpdclient_handle_error(c);
				g_free(filename);
				return -1;
			}
		} else {
			mpdclient_handle_error(c);
			g_free(filename);
			return -1;
		}
	}

	c->events |= MPD_IDLE_STORED_PLAYLIST;

	/* success */
	screen_status_printf(_("Saved %s"), filename);
	g_free(filename);
	return 0;
}
