// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "CustomColors.hxx"
#include "i18n.h"

#include <curses.h>

#include <forward_list>

#include <stdio.h>

struct CustomColor {
	short color;
	short r,g,b;

	constexpr CustomColor(short _color,
			      short _r, short _g, short _b) noexcept
		:color(_color), r(_r), g(_g), b(_b) {}
};

static std::forward_list<CustomColor> custom_colors;

/* This function is called from conf.c before curses have been started,
 * it adds the definition to the color_definition_list and init_color() is
 * done in colors_start() */
void
colors_define(short color, short r, short g, short b) noexcept
{
	custom_colors.emplace_front(color, r, g, b);
}

void
ApplyCustomColors() noexcept
{
	if (custom_colors.empty())
		return;

	if (!can_change_color()) {
		fprintf(stderr, "%s\n",
			_("Terminal lacks support for changing colors"));
		return;
	}

	for (const auto &i : custom_colors)
		if (i.color < COLORS)
			init_color(i.color, i.r, i.g, i.b);
}
