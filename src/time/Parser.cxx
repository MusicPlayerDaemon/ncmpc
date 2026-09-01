// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Parser.hxx"
#include "util/NumberParser.hxx" // for FromChars()
#include "i18n.h"

/**
 * Throws on error.
 */
static time_t
ParseTimeUnit(std::string_view s)
{
	if (s.size() != 1)
		throw _("Unrecognized suffix");

	constexpr time_t MINUTE = 60;
	constexpr time_t HOUR = 60 * MINUTE;
	constexpr time_t DAY = 24 * HOUR;
	constexpr time_t MONTH = 30 * DAY; // TODO: inaccurate
	constexpr time_t YEAR = 365 * DAY; // TODO: inaccurate

	switch (s.front()) {
	case 's':
		return 1;

	case 'M':
		return MINUTE;

	case 'h':
		return HOUR;

	case 'd':
		return DAY;

	case 'm':
		return MONTH;

	case 'y':
	case 'Y':
		return YEAR;

	default:
		throw _("Unrecognized suffix");
	}
}

/**
 * Throws on error.
 */
time_t
ParseDuration(std::string_view s)
{
	time_t value;
	if (const auto result = FromChars(s, value); result.ec != std::errc{})
		throw _("Invalid number");
	else
		s = {result.ptr, s.end()};

	if (!s.empty())
		value *= ParseTimeUnit(s);

	return value;
}
