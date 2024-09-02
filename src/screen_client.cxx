// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_client.hxx"
#include "screen_status.hxx"
#include "mpdclient.hxx"
#include "i18n.h"
#include "charset.hxx"

void
screen_database_update(struct mpdclient &c, const char *path)
{
	assert(c.IsConnected());

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return;

	unsigned id = mpd_run_update(connection, path);
	if (id == 0) {
		if (mpd_connection_get_error(connection) == MPD_ERROR_SERVER &&
		    mpd_connection_get_server_error(connection) == MPD_SERVER_ERROR_UPDATE_ALREADY &&
		    mpd_connection_clear_error(connection))
			screen_status_message(_("Database update running"));
		else
			c.HandleError();
		return;
	}

	if (path != nullptr && *path != 0) {
		screen_status_printf(_("Database update of %s started"),
				     Utf8ToLocaleZ{path}.c_str());
	} else
		screen_status_message(_("Database update started"));
}
