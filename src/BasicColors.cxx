/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "BasicColors.hxx"

#include <curses.h>

#include <strings.h>
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
		if (strcasecmp(basic_color_names[i], name) == 0)
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
