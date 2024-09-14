// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_client.hxx"
#include "client/mpdclient.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "Interface.hxx"

#include <fmt/core.h>

void
screen_database_update(Interface &interface, struct mpdclient &c, const char *path)
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
			interface.Alert(_("Database update running"));
		else
			c.HandleError();
		return;
	}

	if (path != nullptr && *path != 0) {
		interface.Alert(fmt::format(fmt::runtime(_("Database update of {} started")),
					    (std::string_view)Utf8ToLocale{path}));
	} else
		interface.Alert(_("Database update started"));
}
