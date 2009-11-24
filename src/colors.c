/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "colors.h"
#include "i18n.h"
#ifdef ENABLE_COLORS
#include "options.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#define COLOR_NONE  -1
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
	enum color i;

	for (i = 1; i < COLOR_END; ++i)
		if (!strcasecmp(colors[i].name, name))
			return &colors[i];

	return NULL;
}

static int
colors_update_pair(enum color id)
{
	short fg, bg;

	assert(id > 0 && id < COLOR_END);

	fg = colors[id].color;
	bg = colors[COLOR_BACKGROUND].color;

	/* COLOR_NONE is negative, which
	 * results in a default colors */
	init_pair(id, fg, bg);
	return 0;
}

int
colors_str2color(const char *str)
{
	int color;
	char *endptr;

	if (!strcasecmp(str, "black"))
		return COLOR_BLACK;
	else if (!strcasecmp(str, "red"))
		return COLOR_RED;
	else if (!strcasecmp(str, "green"))
		return COLOR_GREEN;
	else if (!strcasecmp(str, "yellow"))
		return COLOR_YELLOW;
	else if (!strcasecmp(str, "blue"))
		return COLOR_BLUE;
	else if (!strcasecmp(str, "magenta"))
		return COLOR_MAGENTA;
	else if (!strcasecmp(str, "cyan"))
		return COLOR_CYAN;
	else if (!strcasecmp(str, "white"))
		return COLOR_WHITE;
	else if (!strcasecmp(str, "brightred"))
		return COLOR_RED | A_BOLD;
	else if (!strcasecmp(str, "brightgreen"))
		return COLOR_GREEN | A_BOLD;
	else if (!strcasecmp(str, "brightyellow"))
		return COLOR_YELLOW | A_BOLD;
	else if (!strcasecmp(str, "brightblue"))
		return COLOR_BLUE | A_BOLD;
	else if (!strcasecmp(str, "brightmagenta"))
		return COLOR_MAGENTA | A_BOLD;
	else if (!strcasecmp(str, "brightcyan"))
		return COLOR_CYAN | A_BOLD;
	else if (!strcasecmp(str, "brightwhite"))
		return COLOR_WHITE | A_BOLD;
	else if (!strcasecmp(str, "grey") || !strcasecmp(str, "gray"))
		return COLOR_BLACK | A_BOLD;
	else if (!strcasecmp(str, "none"))
		return COLOR_NONE;

	color = strtol(str, &endptr, 10);
	if (str != endptr && endptr[0] == '\0')
		return color;

	fprintf(stderr,_("Warning: Unknown color - %s\n"), str);
	return -2;
}

/* This function is called from conf.c before curses have been started,
 * it adds the definition to the color_definition_list and init_color() is
 * done in colors_start() */
int
colors_define(const char *name, short r, short g, short b)
{
	color_definition_entry_t *entry;
	int color = colors_str2color(name);

	if (color < 0)
		return color;

	entry = g_malloc(sizeof(color_definition_entry_t));
	entry->color = color;
	entry->r = r;
	entry->g = g;
	entry->b = b;

	color_definition_list = g_list_append(color_definition_list, entry);

	return 0;
}

int
colors_assign(const char *name, const char *value)
{
	color_entry_t *entry = colors_lookup_by_name(name);
	int color;

	if (!entry) {
		fprintf(stderr,_("Warning: Unknown color field - %s\n"), name);
		return -1;
	}

	if ((color = colors_str2color(value)) == COLOR_ERROR)
		return -1;

	entry->color = color;
	return 0;
}


int
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
			enum color i;

			for (i = 1; i < COLOR_END; ++i)
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
		GList *list = color_definition_list;

		while (list) {
			g_free(list->data);
			list=list->next;
		}

		g_list_free(color_definition_list);
		color_definition_list = NULL;
	}

	return 0;
}
#endif

int
colors_use(WINDOW *w, enum color id)
{
	color_entry_t *entry = &colors[id];
	short pair;
	attr_t attrs;

	assert(id > 0 && id < COLOR_END);

	wattr_get(w, &attrs, &pair, NULL);

#ifdef ENABLE_COLORS
	if (options.enable_colors) {
		/* color mode */
		if ((int)attrs != entry->mono || (short)id != pair)
			wattr_set(w, entry->mono, id, NULL);
	} else {
#endif
		/* mono mode */
		if ((int)attrs != entry->mono)
			(void)wattrset(w, entry->mono);
#ifdef ENABLE_COLORS
	}
#endif

	return 0;
}
