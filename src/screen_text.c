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

#include "screen_text.h"
#include "screen_find.h"
#include "charset.h"

#include <assert.h>
#include <string.h>

void
screen_text_clear(struct screen_text *text)
{
	list_window_reset(text->lw);

	for (guint i = 0; i < text->lines->len; ++i)
		g_free(g_ptr_array_index(text->lines, i));

	g_ptr_array_set_size(text->lines, 0);
	list_window_set_length(text->lw, 0);
}

void
screen_text_set(struct screen_text *text, const GString *str)
{
	const char *p, *eol, *next;

	assert(str != NULL);

	screen_text_clear(text);

	p = str->str;
	while ((eol = strchr(p, '\n')) != NULL) {
		char *line;

		next = eol + 1;

		/* strip whitespace at end */

		while (eol > p && (unsigned char)eol[-1] <= 0x20)
			--eol;

		/* create copy and append it to text->lines */

		line = g_malloc(eol - p + 1);
		memcpy(line, p, eol - p);
		line[eol - p] = 0;

		g_ptr_array_add(text->lines, line);

		/* reset control characters */

		for (eol = line + (eol - p); line < eol; ++line)
			if ((unsigned char)*line < 0x20)
				*line = ' ';

		p = next;
	}

	if (*p != 0)
		g_ptr_array_add(text->lines, g_strdup(p));

	list_window_set_length(text->lw, text->lines->len);
}

const char *
screen_text_list_callback(unsigned idx, G_GNUC_UNUSED bool *highlight,
			  G_GNUC_UNUSED char** sc, void *data)
{
	const struct screen_text *text = data;
	static char buffer[256];
	char *value;

	if (idx >= text->lines->len)
		return NULL;

	value = utf8_to_locale(g_ptr_array_index(text->lines, idx));
	g_strlcpy(buffer, value, sizeof(buffer));
	g_free(value);

	return buffer;
}

bool
screen_text_cmd(struct screen_text *text,
		G_GNUC_UNUSED struct mpdclient *c, command_t cmd)
{
	if (list_window_scroll_cmd(text->lw, cmd)) {
		screen_text_repaint(text);
		return true;
	}

	list_window_set_cursor(text->lw, text->lw->start);
	if (screen_find(text->lw, cmd, screen_text_list_callback, text)) {
		/* center the row */
		list_window_center(text->lw, text->lw->selected);
		screen_text_repaint(text);
		return true;
	}

	return false;
}
