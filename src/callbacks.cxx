// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "callbacks.hxx"
#include "screen_utils.hxx"
#include "screen_status.hxx"
#include "mpdclient.hxx"
#include "ncmpc.hxx"
#include "ui/Bell.hxx"

#include <curses.h>

bool
mpdclient_auth_callback(struct mpdclient *c) noexcept
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	if (!mpd_connection_clear_error(connection))
		return false;

	const auto password = screen_read_password(*screen, nullptr);
	if (password.empty())
		return false;

	mpd_send_password(connection, password.c_str());

	if (!mpd_response_finish(connection)) {
		c->HandleAuthError();
		return false;
	}

	c->Update();
	return true;
}

void
mpdclient_error_callback(std::string_view message) noexcept
{
	screen_status_message(message);
	Bell();
	doupdate();
}

void
mpdclient_error_callback(std::exception_ptr e) noexcept
{
	screen_status_error(std::move(e));
	Bell();
	doupdate();
}
