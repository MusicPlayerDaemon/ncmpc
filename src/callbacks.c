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

#include "callbacks.h"
#include "screen_utils.h"
#include "screen_status.h"
#include "mpdclient.h"
#include "charset.h"

static bool
_screen_auth(struct mpdclient *c, gint recursion)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return false;

	mpd_connection_clear_error(connection);
	if (recursion > 2)
		return false;

	char *password = screen_read_password(NULL);
	if (password == NULL)
		return false;

	mpd_send_password(connection, password);
	g_free(password);

	mpd_response_finish(connection);
	mpdclient_update(c);

	if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
	    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_PASSWORD)
		return  _screen_auth(c, ++recursion);

	return true;
}

bool
screen_auth(struct mpdclient *c)
{
	return _screen_auth(c, 0);
}

void
mpdclient_ui_error(const char *message_utf8)
{
	char *message_locale = utf8_to_locale(message_utf8);
	screen_status_printf("%s", message_locale);
	g_free(message_locale);

	screen_bell();
	doupdate();
}
