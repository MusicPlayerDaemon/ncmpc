/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "colors.h"
#include "i18n.h"
#include "ncfix.h"

#ifdef ENABLE_COLORS
#include "options.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#define COLOR_NONE  G_MININT /* left most bit only */
#define COLOR_ERROR -2

#ifdef ENABLE_COLORS
typedef struct {
	short color;
	short r,g,b;
} color_definition_entry_t;
#endif

typedef struct {
	const char *name;
	int color;
	int mono;
} color_entry_t;

static color_entry_t colors[COLOR_END] = {
	/* color pair = field name, color, mono */
	[COLOR_TITLE]        = {"title",             COLOR_YELLOW,          A_NORMAL},
	[COLOR_TITLE_BOLD]   = {"title-bold",        COLOR_YELLOW | A_BOLD, A_BOLD  },
	[COLOR_LINE]         = {"line",              COLOR_WHITE,           A_NORMAL},
	[COLOR_LINE_BOLD]    = {"line-bold",         COLOR_WHITE  | A_BOLD, A_BOLD  },
	[COLOR_LINE_FLAGS]   = {"line-flags",        COLOR_YELLOW,          A_NORMAL},
	[COLOR_LIST]         = {"list",              COLOR_GREEN,           A_NORMAL},
	[COLOR_LIST_BOLD]    = {"list-bold",         COLOR_GREEN  | A_BOLD, A_BOLD  },
	[COLOR_PROGRESSBAR]  = {"progressbar",       COLOR_WHITE,           A_NORMAL},
	[COLOR_STATUS]       = {"status-song",       COLOR_YELLOW,          A_NORMAL},
	[COLOR_STATUS_BOLD]  = {"status-state",      COLOR_YELLOW | A_BOLD, A_BOLD  },
	[COLOR_STATUS_TIME]  = {"status-time",       COLOR_RED,             A_NORMAL},
	[COLOR_STATUS_ALERT] = {"alert",             COLOR_RED    | A_BOLD, A_BOLD  },
	[COLOR_DIRECTORY]    = {"browser-directory", COLOR_YELLOW,          A_NORMAL},
	[COLOR_PLAYLIST]     = {"browser-playlist",  COLOR_RED,             A_NORMAL},
	[COLOR_BACKGROUND]   = {"background",        COLOR_BLACK,           A_NORMAL},
};

#ifdef ENABLE_COLORS

static GList *color_definition_list = NULL;

static color_entry_t *
colors_lookup_by_name(const char *name)
{
	for (enum color i = 1; i < COLOR_END; ++i)
		if (!strcasecmp(colors[i].name, name))
			return &colors[i];

	return NULL;
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

int
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
		if (!strcasecmp(cur, "none"))
			color |= COLOR_NONE;
		else if (!strcasecmp(cur, "black"))
			color |= COLOR_BLACK;
		else if (!strcasecmp(cur, "red"))
			color |= COLOR_RED;
		else if (!strcasecmp(cur, "green"))
			color |= COLOR_GREEN;
		else if (!strcasecmp(cur, "yellow"))
			color |= COLOR_YELLOW;
		else if (!strcasecmp(cur, "blue"))
			color |= COLOR_BLUE;
		else if (!strcasecmp(cur, "magenta"))
			color |= COLOR_MAGENTA;
		else if (!strcasecmp(cur, "cyan"))
			color |= COLOR_CYAN;
		else if (!strcasecmp(cur, "white"))
			color |= COLOR_WHITE;
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

/* This function is called from conf.c before curses have been started,
 * it adds the definition to the color_definition_list and init_color() is
 * done in colors_start() */
bool
colors_define(const char *name, short r, short g, short b)
{
	int color = colors_str2color(name);

	if (color < 0)
		return false;

	color_definition_entry_t *entry =
		g_malloc(sizeof(color_definition_entry_t));
	entry->color = color;
	entry->r = r;
	entry->g = g;
	entry->b = b;

	color_definition_list = g_list_append(color_definition_list, entry);

	return true;
}

bool
colors_assign(const char *name, const char *value)
{
	color_entry_t *entry = colors_lookup_by_name(name);

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
colors_start(void)
{
	if (has_colors()) {
		/* initialize color support */
		start_color();
		use_default_colors();
		/* define any custom colors defined in the configuration file */
		if (color_definition_list && can_change_color()) {
			GList *list = color_definition_list;

			while (list) {
				color_definition_entry_t *entry = list->data;

				if (entry->color <= COLORS)
					init_color(entry->color, entry->r,
						   entry->g, entry->b);
				list = list->next;
			}
		} else if (color_definition_list && !can_change_color())
			fprintf(stderr, "%s\n",
				_("Terminal lacks support for changing colors"));

		if (options.enable_colors) {
			for (enum color i = 1; i < COLOR_END; ++i)
				/* update the color pairs */
				colors_update_pair(i);
		}
	} else if (options.enable_colors) {
		fprintf(stderr, "%s\n",
			_("Terminal lacks color capabilities"));
		options.enable_colors = 0;
	}

	/* free the color_definition_list */
	if (color_definition_list) {
		g_list_free_full(color_definition_list, g_free);
		color_definition_list = NULL;
	}
}
#endif

void
colors_use(WINDOW *w, enum color id)
{
	color_entry_t *entry = &colors[id];

	assert(id > 0 && id < COLOR_END);

	attr_t attrs;
	short pair;
	fix_wattr_get(w, &attrs, &pair, NULL);

#ifdef ENABLE_COLORS
	if (options.enable_colors) {
		/* color mode */
		if ((int)attrs != entry->color || (short)id != pair)
			wattr_set(w, entry->color, id, NULL);
	} else {
#endif
		/* mono mode */
		if ((int)attrs != entry->mono)
			(void)wattrset(w, entry->mono);
#ifdef ENABLE_COLORS
	}
#endif
}
