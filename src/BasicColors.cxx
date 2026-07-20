// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "BasicColors.hxx"
#include "util/NumberParser.hxx"
#include "util/StringCompare.hxx"

#include <curses.h>

static constexpr std::string_view basic_color_names[] = {
	"black",
	"red",
	"green",
	"yellow",
	"blue",
	"magenta",
	"cyan",
	"white",
	{}
};

static_assert(COLOR_BLACK == 0, "Unexpected color value");
static_assert(COLOR_RED == 1, "Unexpected color value");
static_assert(COLOR_GREEN == 2, "Unexpected color value");
static_assert(COLOR_YELLOW == 3, "Unexpected color value");
static_assert(COLOR_BLUE == 4, "Unexpected color value");
static_assert(COLOR_MAGENTA == 5, "Unexpected color value");
static_assert(COLOR_CYAN == 6, "Unexpected color value");
static_assert(COLOR_WHITE == 7, "Unexpected color value");

short
ParseBasicColorName(std::string_view name) noexcept
{
	for (size_t i = 0; !basic_color_names[i].empty(); ++i)
		if (StringIsEqualIgnoreCase(basic_color_names[i], name))
			return i;

	return -1;
}

short
ParseColorNameOrNumber(std::string_view s) noexcept
{
	if (short basic = ParseBasicColorName(s); basic >= 0)
		return basic;

	if (short result; ParseIntegerTo(s, result, 10) && result <= 0xff)
		return result;

	return -1;
}
