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

#include "hscroll.h"
#include "charset.h"
#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#ifndef NCMPC_MINI

// FIXME: utf-8 length
char *
strscroll(char *str, char *separator, int width, scroll_state_t *st)
{
	gchar *tmp, *buf;
	gsize len, size;

	assert(str != NULL);
	assert(separator != NULL);
	assert(st != NULL);

	if( st->offset==0 ) {
		st->offset++;
		return g_strdup(str);
	}

	/* create a buffer containing the string and the separator */
	size = strlen(str)+strlen(separator)+1;
	tmp = g_malloc(size);
	g_strlcpy(tmp, str, size);
	g_strlcat(tmp, separator, size);
	len = utf8_width(tmp);

	if (st->offset >= len)
		st->offset = 0;

	/* create the new scrolled string */
	size = width+1;
	if (g_utf8_validate(tmp, -1, NULL) ) {
		int ulen;
		buf = g_malloc(size*6);// max length of utf8 char is 6
		g_utf8_strncpy(buf, g_utf8_offset_to_pointer(tmp,st->offset), size);
		if( (ulen = g_utf8_strlen(buf, -1)) < width )
			g_utf8_strncpy(buf+strlen(buf), tmp, size - ulen - 1);
	} else {
		buf = g_malloc(size);
		g_strlcpy(buf, tmp+st->offset, size);
		if (strlen(buf) < (size_t)width)
			g_strlcat(buf, tmp, size);
	}
	if( time(NULL)-st->t >= 1 ) {
		st->t = time(NULL);
		st->offset++;
	}
	g_free(tmp);
	return buf;
}

#endif
