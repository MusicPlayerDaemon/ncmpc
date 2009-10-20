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

#include "hscroll.h"
#include "charset.h"

#include <assert.h>
#include <string.h>

char *
strscroll(struct hscroll *hscroll, const char *str, const char *separator,
	  unsigned width)
{
	gchar *tmp, *buf;

	assert(hscroll != NULL);
	assert(str != NULL);
	assert(separator != NULL);

	/* create a buffer containing the string and the separator */
	tmp = replace_locale_to_utf8(g_strconcat(str, separator,
						 str, separator, NULL));

	if (hscroll->offset >= (unsigned)g_utf8_strlen(tmp, -1) / 2)
		hscroll->offset = 0;

	/* create the new scrolled string */
	buf = g_utf8_offset_to_pointer(tmp, hscroll->offset);
	utf8_cut_width(buf, width);

	/* convert back to locale */
	buf = utf8_to_locale(buf);
	g_free(tmp);
	return buf;
}
