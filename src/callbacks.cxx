// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Instance.hxx"
#include "ui/Bell.hxx"
#include "util/Exception.hxx"

#include <curses.h>

bool
Instance::OnMpdAuth() noexcept
{
	auto *connection = client.GetConnection();
	if (connection == nullptr)
		return false;

	if (!mpd_connection_clear_error(connection))
		return false;

	screen_manager.QueryPassword(client);
	return false;
}

void
Instance::OnMpdError(std::string_view message) noexcept
{
	screen_manager.Alert(std::string{message});
	Bell();
	doupdate();
}

void
Instance::OnMpdError(std::exception_ptr e) noexcept
{
	screen_manager.Alert(GetFullMessage(std::move(e)));
	Bell();
	doupdate();
}
