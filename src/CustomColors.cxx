/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "CustomColors.hxx"
#include "config.h"
#include "ncmpc_curses.h"
#include "i18n.h"

#include <list>

#include <stdio.h>

struct CustomColor {
	short color;
	short r,g,b;

	constexpr CustomColor(short _color, short _r, short _g, short _b)
		:color(_color), r(_r), g(_g), b(_b) {}
};

static std::list<CustomColor> custom_colors;

/* This function is called from conf.c before curses have been started,
 * it adds the definition to the color_definition_list and init_color() is
 * done in colors_start() */
void
colors_define(short color, short r, short g, short b)
{
	custom_colors.emplace_back(color, r, g, b);
}

void
ApplyCustomColors()
{
	if (custom_colors.empty())
		return;

	if (!can_change_color()) {
		fprintf(stderr, "%s\n",
			_("Terminal lacks support for changing colors"));
		return;
	}

	for (const auto &i : custom_colors)
		if (i.color <= COLORS)
			init_color(i.color, i.r, i.g, i.b);
}
