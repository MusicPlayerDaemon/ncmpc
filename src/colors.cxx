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

#include "colors.hxx"
#include "BasicColors.hxx"
#include "CustomColors.hxx"
#include "i18n.h"

#ifdef ENABLE_COLORS
#include "options.hxx"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#define COLOR_NONE  G_MININT /* left most bit only */
#define COLOR_ERROR -2

struct NamedColor {
	const char *name;
#ifdef ENABLE_COLORS
	int color;
#endif
	int mono;

#ifndef ENABLE_COLORS
	constexpr NamedColor(const char *_name, int, int _mono)
		:name(_name), mono(_mono) {}
#endif
};

static NamedColor colors[COLOR_END] = {
	/* color pair = field name, color, mono */
	{nullptr, 0, 0},
	{"title",             COLOR_YELLOW,          A_NORMAL},
	{"title-bold",        COLOR_YELLOW | A_BOLD, A_BOLD  },
	{"line",              COLOR_WHITE,           A_NORMAL},
	{"line-bold",         COLOR_WHITE  | A_BOLD, A_BOLD  },
	{"line-flags",        COLOR_YELLOW,          A_NORMAL},
	{"list",              COLOR_GREEN,           A_NORMAL},
	{"list-bold",         COLOR_GREEN  | A_BOLD, A_BOLD  },
	{"progressbar",       COLOR_WHITE,           A_NORMAL},
	{"progressbar-background", COLOR_BLACK | A_BOLD, A_NORMAL},
	{"status-song",       COLOR_YELLOW,          A_NORMAL},
	{"status-state",      COLOR_YELLOW | A_BOLD, A_BOLD  },
	{"status-time",       COLOR_RED,             A_NORMAL},
	{"alert",             COLOR_RED    | A_BOLD, A_BOLD  },
	{"browser-directory", COLOR_YELLOW,          A_NORMAL},
	{"browser-playlist",  COLOR_RED,             A_NORMAL},
	{"background",        COLOR_BLACK,           A_NORMAL},
};

#ifdef ENABLE_COLORS

static NamedColor *
colors_lookup_by_name(const char *name)
{
	for (unsigned i = 1; i < COLOR_END; ++i)
		if (!strcasecmp(colors[i].name, name))
			return &colors[i];

	return nullptr;
}

static void
colors_update_pair(enum color id)
{
	assert(id > 0 && id < COLOR_END);

	int fg = colors[id].color;
	int bg = colors[COLOR_BACKGROUND].color;

	/* If color == COLOR_NONE (negative),
	 * pass -1 to avoid cast errors */
	init_pair(id,
		(fg < 0 ? -1 : fg),
		(bg < 0 ? -1 : bg));
}

gcc_pure
static int
colors_str2color(const char *str)
{
	int color = 0;
	char **parts = g_strsplit(str, ",", 0);
	for (int i = 0; parts[i]; i++) {
		char *cur = parts[i];

		/* Legacy colors (brightblue,etc) */
		if (!strncasecmp(cur, "bright", 6)) {
			color |= A_BOLD;
			cur += 6;
		}

		/* Colors */
		short b = ParseBasicColorName(cur);
		if (b >= 0) {
			color |= b;
			continue;
		}

		if (!strcasecmp(cur, "none"))
			color |= COLOR_NONE;
		else if (!strcasecmp(cur, "grey") || !strcasecmp(cur, "gray"))
			color |= COLOR_BLACK | A_BOLD;

		/* Attributes */
		else if (!strcasecmp(cur, "standout"))
			color |= A_STANDOUT;
		else if (!strcasecmp(cur, "underline"))
			color |= A_UNDERLINE;
		else if (!strcasecmp(cur, "reverse"))
			color |= A_REVERSE;
		else if (!strcasecmp(cur, "blink"))
			color |= A_BLINK;
		else if (!strcasecmp(cur, "dim"))
			color |= A_DIM;
		else if (!strcasecmp(cur, "bold"))
			color |= A_BOLD;
		else {
			/* Numerical colors */
			char *endptr;
			int tmp = strtol(cur, &endptr, 10);
			if (cur != endptr && endptr[0] == '\0') {
				color |= tmp;
			} else {
				fprintf(stderr, "%s: %s\n",
					_("Unknown color"), str);
				return COLOR_ERROR;
			}
		}

	}
	g_strfreev(parts);
	return color;
}

bool
colors_assign(const char *name, const char *value)
{
	auto *entry = colors_lookup_by_name(name);

	if (!entry) {
		fprintf(stderr, "%s: %s",
			_("Unknown color field"), name);
		return false;
	}

	const int color = colors_str2color(value);
	if (color == COLOR_ERROR)
		return false;

	entry->color = color;
	return true;
}

void
colors_start()
{
	if (has_colors()) {
		/* initialize color support */
		start_color();
		use_default_colors();
		/* define any custom colors defined in the configuration file */
		ApplyCustomColors();

		if (options.enable_colors) {
			for (unsigned i = 1; i < COLOR_END; ++i)
				/* update the color pairs */
				colors_update_pair((enum color)i);
		}
	} else if (options.enable_colors) {
		fprintf(stderr, "%s\n",
			_("Terminal lacks color capabilities"));
		options.enable_colors = false;
	}
}
#endif

void
colors_use(WINDOW *w, enum color id)
{
	auto *entry = &colors[id];

	assert(id > 0 && id < COLOR_END);

#ifdef ENABLE_COLORS
	if (options.enable_colors) {
		/* color mode */
		wattr_set(w, entry->color, id, nullptr);
	} else {
#endif
		/* mono mode */
		(void)wattrset(w, entry->mono);
#ifdef ENABLE_COLORS
	}
#endif
}
