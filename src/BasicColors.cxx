// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "BasicColors.hxx"
#include "util/StringAPI.hxx"

#include <curses.h>

#include <stdlib.h>

static constexpr const char *basic_color_names[] = {
	"black",
	"red",
	"green",
	"yellow",
	"blue",
	"magenta",
	"cyan",
	"white",
	nullptr
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
ParseBasicColorName(const char *name) noexcept
{
	for (size_t i = 0; basic_color_names[i] != nullptr; ++i)
		if (StringIsEqualIgnoreCase(basic_color_names[i], name))
			return i;

	return -1;
}

short
ParseColorNameOrNumber(const char *s) noexcept
{
	short basic = ParseBasicColorName(s);
	if (basic >= 0)
		return basic;

	char *endptr;
	long numeric = strtol(s, &endptr, 10);
	if (endptr > s && *endptr == 0 && numeric >= 0 && numeric <= 0xff)
		return numeric;

	return -1;
}
