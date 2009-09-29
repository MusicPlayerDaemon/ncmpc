/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#include "screen_client.h"
#include "screen_utils.h"
#include "mpdclient.h"

static gint
_screen_auth(struct mpdclient *c, gint recursion)
{
	char *password;

	mpd_connection_clear_error(c->connection);
	if (recursion > 2)
		return 1;

	password = screen_read_password(NULL, NULL);
	if (password == NULL)
		return 1;

	mpd_send_password(c->connection, password);
	g_free(password);

	mpd_response_finish(c->connection);
	mpdclient_update(c);

	if (mpd_connection_get_error(c->connection) == MPD_ERROR_SERVER &&
	    mpd_connection_get_server_error(c->connection) == MPD_SERVER_ERROR_PASSWORD)
		return  _screen_auth(c, ++recursion);
	return 0;
}

gint
screen_auth(struct mpdclient *c)
{
	return _screen_auth(c, 0);
}
