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

#include "screen_client.hxx"
#include "screen_status.hxx"
#include "mpdclient.hxx"
#include "i18n.h"
#include "charset.hxx"

#include <glib.h>

void
screen_database_update(struct mpdclient *c, const char *path)
{
	assert(c != nullptr);
	assert(c->IsConnected());

	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return;

	unsigned id = mpd_run_update(connection, path);
	if (id == 0) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_UPDATE_ALREADY &&
		    mpd_connection_clear_error(connection))
			screen_status_printf(_("Database update running..."));
		else
			mpdclient_handle_error(c);
		return;
	}

	if (path != nullptr && *path != 0) {
		char *path_locale = utf8_to_locale(path);
		screen_status_printf(_("Database update of %s started"), path);
		g_free(path_locale);
	} else
		screen_status_message(_("Database update started"));
}
