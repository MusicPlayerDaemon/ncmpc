// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "time_format.hxx"
#include "i18n.h"
#include "lib/fmt/ToSpan.hxx"

#include <stdio.h>

using std::string_view_literals::operator""sv;

std::string_view
format_duration_short(std::span<char> buffer, unsigned duration) noexcept
{
	if (duration < 3600)
		return FmtTruncate(buffer, "{}:{:02}"sv, duration / 60, duration % 60);
	else
		return FmtTruncate(buffer, "{}:{:02}:{:02}"sv,
				   duration / 3600,
				   (duration % 3600) / 60,
				   duration % 60);
}

void
format_duration_long(std::span<char> buffer, unsigned long duration) noexcept
{
	unsigned bytes_written = 0;

	if (duration / 31536000 > 0) {
		if (duration / 31536000 == 1)
			bytes_written = snprintf(buffer.data(), buffer.size(),  "%d %s, ", 1, _("year"));
		else
			bytes_written = snprintf(buffer.data(), buffer.size(),  "%lu %s, ", duration / 31536000, _("years"));
		duration %= 31536000;
		buffer = buffer.subspan(bytes_written);
	}
	if (duration / 604800 > 0) {
		if (duration / 604800 == 1)
			bytes_written = snprintf(buffer.data(), buffer.size(),  "%d %s, ",
						 1, _("week"));
		else
			bytes_written = snprintf(buffer.data(), buffer.size(),  "%lu %s, ",
						 duration / 604800, _("weeks"));
		duration %= 604800;
		buffer = buffer.subspan(bytes_written);
	}
	if (duration / 86400 > 0) {
		if (duration / 86400 == 1)
			bytes_written = snprintf(buffer.data(), buffer.size(),  "%d %s, ",
						 1, _("day"));
		else
			bytes_written = snprintf(buffer.data(), buffer.size(),  "%lu %s, ",
						 duration / 86400, _("days"));
		duration %= 86400;
		buffer = buffer.subspan(bytes_written);
	}

	snprintf(buffer.data(), buffer.size(),  "%02lu:%02lu:%02lu", duration / 3600,
		 duration % 3600 / 60, duration % 3600 % 60);
}
