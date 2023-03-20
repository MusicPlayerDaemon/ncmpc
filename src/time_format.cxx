// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "time_format.hxx"
#include "i18n.h"

#include <stdio.h>

void
format_duration_short(char *buffer, size_t length, unsigned duration) noexcept
{
	if (duration < 3600)
		snprintf(buffer, length,
			 "%i:%02i", duration / 60, duration % 60);
	else
		snprintf(buffer, length,
			 "%i:%02i:%02i", duration / 3600,
			 (duration % 3600) / 60, duration % 60);
}

void
format_duration_long(char *p, size_t length, unsigned long duration) noexcept
{
	unsigned bytes_written = 0;

	if (duration / 31536000 > 0) {
		if (duration / 31536000 == 1)
			bytes_written = snprintf(p, length, "%d %s, ", 1, _("year"));
		else
			bytes_written = snprintf(p, length, "%lu %s, ", duration / 31536000, _("years"));
		duration %= 31536000;
		length -= bytes_written;
		p += bytes_written;
	}
	if (duration / 604800 > 0) {
		if (duration / 604800 == 1)
			bytes_written = snprintf(p, length, "%d %s, ",
						 1, _("week"));
		else
			bytes_written = snprintf(p, length, "%lu %s, ",
						 duration / 604800, _("weeks"));
		duration %= 604800;
		length -= bytes_written;
		p += bytes_written;
	}
	if (duration / 86400 > 0) {
		if (duration / 86400 == 1)
			bytes_written = snprintf(p, length, "%d %s, ",
						 1, _("day"));
		else
			bytes_written = snprintf(p, length, "%lu %s, ",
						 duration / 86400, _("days"));
		duration %= 86400;
		length -= bytes_written;
		p += bytes_written;
	}

	snprintf(p, length, "%02lu:%02lu:%02lu", duration / 3600,
		 duration % 3600 / 60, duration % 3600 % 60);
}
