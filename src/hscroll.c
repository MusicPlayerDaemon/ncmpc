/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#include "hscroll.h"
#include "charset.h"
#include "ncfix.h"

#include <assert.h>
#include <string.h>

char *
strscroll(struct hscroll *hscroll, const char *str, const char *separator,
	  unsigned width)
{
	assert(hscroll != NULL);
	assert(str != NULL);
	assert(separator != NULL);

	/* create a buffer containing the string and the separator */
	char *tmp = replace_locale_to_utf8(g_strconcat(str, separator,
						       str, separator, NULL));
	if (hscroll->offset >= (unsigned)g_utf8_strlen(tmp, -1) / 2)
		hscroll->offset = 0;

	/* create the new scrolled string */
	char *buf = g_utf8_offset_to_pointer(tmp, hscroll->offset);
	utf8_cut_width(buf, width);

	/* convert back to locale */
	buf = utf8_to_locale(buf);
	g_free(tmp);
	return buf;
}

/**
 * This timer scrolls the area by one and redraws.
 */
static gboolean
hscroll_timer_callback(gpointer data)
{
	struct hscroll *hscroll = data;

	hscroll_step(hscroll);
	hscroll_draw(hscroll);
	wrefresh(hscroll->w);
	return true;
}

void
hscroll_set(struct hscroll *hscroll, unsigned x, unsigned y, unsigned width,
	    const char *text)
{
	assert(hscroll != NULL);
	assert(hscroll->w != NULL);
	assert(text != NULL);

	if (hscroll->text != NULL && hscroll->x == x && hscroll->y == y &&
	    hscroll->width == width && strcmp(hscroll->text, text) == 0)
		/* no change, do nothing (and, most importantly, do
		   not reset the current offset!) */
		return;

	hscroll_clear(hscroll);

	hscroll->x = x;
	hscroll->y = y;
	hscroll->width = width;

	/* obtain the ncurses attributes and the current color, store
	   them */
	fix_wattr_get(hscroll->w, &hscroll->attrs, &hscroll->pair, NULL);

	hscroll->text = g_strdup(text);
	hscroll->offset = 0;
	hscroll->source_id = g_timeout_add_seconds(1, hscroll_timer_callback,
						   hscroll);
}

void
hscroll_clear(struct hscroll *hscroll)
{
	assert(hscroll != NULL);

	if (hscroll->text == NULL)
		return;

	g_source_remove(hscroll->source_id);

	g_free(hscroll->text);
	hscroll->text = NULL;
}

void
hscroll_draw(struct hscroll *hscroll)
{
	assert(hscroll != NULL);
	assert(hscroll->w != NULL);
	assert(hscroll->text != NULL);

	/* set stored attributes and color */
	attr_t old_attrs;
	short old_pair;
	fix_wattr_get(hscroll->w, &old_attrs, &old_pair, NULL);
	wattr_set(hscroll->w, hscroll->attrs, hscroll->pair, NULL);

	/* scroll the string, and draw it */
	char *p = strscroll(hscroll, hscroll->text, hscroll->separator,
			    hscroll->width);
	mvwaddstr(hscroll->w, hscroll->y, hscroll->x, p);
	g_free(p);

	/* restore previous attributes and color */
	wattr_set(hscroll->w, old_attrs, old_pair, NULL);
}
