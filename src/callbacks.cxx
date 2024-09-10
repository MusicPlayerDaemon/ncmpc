// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Instance.hxx"
#include "screen_status.hxx"
#include "ui/Bell.hxx"

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
	screen_status_message(message);
	Bell();
	doupdate();
}

void
Instance::OnMpdError(std::exception_ptr e) noexcept
{
	screen_status_error(std::move(e));
	Bell();
	doupdate();
}
