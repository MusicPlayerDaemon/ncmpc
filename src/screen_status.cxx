// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_status.hxx"
#include "screen.hxx"
#include "ncmpc.hxx"
#include "util/Exception.hxx"

#include <stdarg.h>

void
screen_status_message(std::string &&msg) noexcept
{
	screen->status_bar.SetMessage(std::move(msg));
}

void
screen_status_message(const char *msg) noexcept
{
	screen_status_message(std::string{msg});
}

void
screen_status_printf(const char *format, ...) noexcept
{
	va_list ap;
	va_start(ap,format);
	char msg[256];
	vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);
	screen_status_message(msg);
}

void
screen_status_error(std::exception_ptr e) noexcept
{
	screen_status_message(GetFullMessage(std::move(e)));
}
