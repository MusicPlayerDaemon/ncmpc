/*
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "colors.h"
#include "i18n.h"
#include "options.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#define COLOR_BRIGHT_MASK (1<<7)

#define COLOR_BRIGHT_BLACK (COLOR_BLACK | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_RED (COLOR_RED | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_GREEN (COLOR_GREEN | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_YELLOW (COLOR_YELLOW | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_BLUE (COLOR_BLUE | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_MAGENTA (COLOR_MAGENTA | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_CYAN (COLOR_CYAN | COLOR_BRIGHT_MASK)
#define COLOR_BRIGHT_WHITE (COLOR_WHITE | COLOR_BRIGHT_MASK)

#define IS_BRIGHT(color) (color & COLOR_BRIGHT_MASK)

/* name of the color fields */
#define NAME_TITLE "title"
#define NAME_TITLE_BOLD "title-bold"
#define NAME_LINE "line"
#define NAME_LINE_BOLD "line-flags"
#define NAME_LIST "list"
#define NAME_LIST_BOLD "list-bold"
#define NAME_PROGRESS "progressbar"
#define NAME_STATUS "status-song"
#define NAME_STATUS_BOLD "status-state"
#define NAME_STATUS_TIME "status-time"
#define NAME_ALERT "alert"
#define NAME_BGCOLOR "background"

typedef struct {
	short color;
	short r,g,b;
} color_definition_entry_t;

typedef struct {
	const char *name;
	short fg;
	attr_t attrs;
} color_entry_t;

static color_entry_t colors[COLOR_END] = {
	/* color pair = field name, color, mono attribute */
	[COLOR_TITLE] = { NAME_TITLE, COLOR_YELLOW, A_NORMAL },
	[COLOR_TITLE_BOLD] = { NAME_TITLE_BOLD, COLOR_BRIGHT_YELLOW, A_BOLD },
	[COLOR_LINE] = { NAME_LINE, COLOR_WHITE, A_NORMAL },
	[COLOR_LINE_BOLD] = { NAME_LINE_BOLD, COLOR_BRIGHT_WHITE, A_BOLD },
	[COLOR_LIST] = { NAME_LIST, COLOR_GREEN, A_NORMAL },
	[COLOR_LIST_BOLD] = { NAME_LIST_BOLD, COLOR_BRIGHT_GREEN, A_BOLD },
	[COLOR_PROGRESSBAR] = { NAME_PROGRESS, COLOR_WHITE, A_NORMAL },
	[COLOR_STATUS] = { NAME_STATUS, COLOR_YELLOW, A_NORMAL },
	[COLOR_STATUS_BOLD] = { NAME_STATUS_BOLD, COLOR_BRIGHT_YELLOW, A_BOLD },
	[COLOR_STATUS_TIME] = { NAME_STATUS_TIME, COLOR_RED, A_NORMAL },
	[COLOR_STATUS_ALERT] = { NAME_ALERT, COLOR_BRIGHT_RED, A_BOLD },
};

/* background color */
static short bg = COLOR_BLACK;

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
	color_entry_t *entry = &colors[id];
	short fg = -1;

	assert(id > 0 && id < COLOR_END);

	if (IS_BRIGHT(entry->fg)) {
		entry->attrs = A_BOLD;
		fg = entry->fg & ~COLOR_BRIGHT_MASK;
	} else {
		entry->attrs = A_NORMAL;
		fg = entry->fg;
	}

	init_pair(id, fg, bg);
	return 0;
}

short
colors_str2color(const char *str)
{
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
		return COLOR_BRIGHT_RED;
	else if (!strcasecmp(str, "brightgreen"))
		return COLOR_BRIGHT_GREEN;
	else if (!strcasecmp(str, "brightyellow"))
		return COLOR_BRIGHT_YELLOW;
	else if (!strcasecmp(str, "brightblue"))
		return COLOR_BRIGHT_BLUE;
	else if (!strcasecmp(str, "brightmagenta"))
		return COLOR_BRIGHT_MAGENTA;
	else if (!strcasecmp(str, "brightcyan"))
		return COLOR_BRIGHT_CYAN;
	else if (!strcasecmp(str, "brightwhite"))
		return COLOR_BRIGHT_WHITE;
	else if (!strcasecmp(str, "grey") || !strcasecmp(str, "gray"))
		return COLOR_BRIGHT_BLACK;
	else if (!strcasecmp(str, "none"))
		return -1;

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
	short color = colors_str2color(name);

	if (color < 0)
		return -1;

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
	short color;

	if (!entry) {
		if (!strcasecmp(NAME_BGCOLOR, name)) {
			if ((color = colors_str2color(value)) < -1)
				return -1;
			if (color > COLORS)
				color = color & ~COLOR_BRIGHT_MASK;
			bg = color;
			return 0;
		}

		fprintf(stderr,_("Warning: Unknown color field - %s\n"), name);
		return -1;
	}

	if ((color = colors_str2color(value)) < -1)
		return -1;

	entry->fg = color;
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
			fprintf(stderr, _("Terminal lacks support for changing colors!\n"));

		if (options.enable_colors) {
			enum color i;

			for (i = 1; i < COLOR_END; ++i)
				/* update the color pairs */
				colors_update_pair(i);
		}
	} else if (options.enable_colors) {
		fprintf(stderr, _("Terminal lacks color capabilities!\n"));
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

int
colors_use(WINDOW *w, enum color id)
{
	color_entry_t *entry = &colors[id];
	short pair;
	attr_t attrs;

	assert(id > 0 && id < COLOR_END);

	wattr_get(w, &attrs, &pair, NULL);

	if (options.enable_colors) {
		/* color mode */
		if (attrs != entry->attrs || (short)id != pair)
			wattr_set(w, entry->attrs, id, NULL);
	} else {
		/* mono mode */
		if (attrs != entry->attrs)
			wattrset(w, entry->attrs);
	}

	return 0;
}
