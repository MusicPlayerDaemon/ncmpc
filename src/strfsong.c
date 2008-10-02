/* 
 * $Id$
 *
 * Based on mpc's songToFormatedString modified for glib and ncmpc
 *
 *
 * (c) 2003-2004 by normalperson and Warren Dukes (shank@mercury.chem.pitt.edu)
 *              and Daniel Brown (danb@cs.utexas.edu)
 *              and Kalle Wallin (kaw@linux.se)
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
 */

#include "strfsong.h"
#include "support.h"

#include <string.h>

static const gchar *
skip(const gchar * p)
{
	gint stack = 0;

	while (*p != '\0') {
		if(*p == '[') stack++;
		if(*p == '#' && p[1] != '\0') {
			/* skip escaped stuff */
			++p;
		} else if(stack) {
			if(*p == ']') stack--;
		} else {
			if(*p == '&' || *p == '|' || *p == ']') {
				break;
			}
		}
		++p;
	}

	return p;
}

static gsize
_strfsong(gchar *s,
	  gsize max,
	  const gchar *format,
	  const struct mpd_song *song,
	  const gchar **last)
{
	const gchar *p, *end;
	gchar *temp;
	gsize n, length = 0;
	gboolean found = FALSE;

	memset(s, 0, max);
	if( song==NULL )
		return 0;

	for (p = format; *p != '\0' && length<max;) {
		/* OR */
		if (p[0] == '|') {
			++p;
			if(!found) {
				memset(s, 0, max);
				length = 0;
			} else {
				p = skip(p);
			}
			continue;
		}

		/* AND */
		if (p[0] == '&') {
			++p;
			if(!found) {
				p = skip(p);
			} else {
				found = FALSE;
			}
			continue;
		}

		/* EXPRESSION START */
		if (p[0] == '[') {
			temp = g_malloc0(max);
			if( _strfsong(temp, max, p+1, song, &p) >0 ) {
				g_strlcat(s, temp, max);
				length = strlen(s);
				found = TRUE;
			}
			g_free(temp);
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']') {
			if(last) *last = p+1;
			if(!found && length) {
				memset(s, 0, max);
				length = 0;
			}
			return length;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '#' && p[0] != '%' && length<max) {
			s[length++] = *p;
			p++;
			continue;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0' && length<max) {
			s[length++] = *(p+1);
			p+=2;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		temp = NULL;
		end  = p+1;
		while(*end >= 'a' && *end <= 'z') {
			end++;
		}
		n = end - p + 1;
		if(*end != '%')
			n--;
		else if (strncmp("%file%", p, n) == 0)
			temp = utf8_to_locale(song->file);
		else if (strncmp("%artist%", p, n) == 0)
			temp = song->artist ? utf8_to_locale(song->artist) : NULL;
		else if (strncmp("%title%", p, n) == 0)
			temp = song->title ? utf8_to_locale(song->title) : NULL;
		else if (strncmp("%album%", p, n) == 0)
			temp = song->album ? utf8_to_locale(song->album) : NULL;
		else if (strncmp("%shortalbum%", p, n) == 0) {
			temp = song->album ? utf8_to_locale(song->album) : NULL;
			if (temp) {
				gchar *temp2 = g_strndup(temp, 25);
				if (strlen(temp) > 25) {
					temp2[24] = '.';
					temp2[23] = '.';
					temp2[22] = '.';
				}
				g_free(temp);
				temp = temp2;
			}
		}
		else if (strncmp("%track%", p, n) == 0)
			temp = song->track ? utf8_to_locale(song->track) : NULL;
		else if (strncmp("%name%", p, n) == 0)
			temp = song->name ? utf8_to_locale(song->name) : NULL;
		else if (strncmp("%date%", p, n) == 0)
			temp = song->date ? utf8_to_locale(song->date) : NULL;
		else if (strncmp("%genre%", p, n) == 0)
			temp = song->genre ? utf8_to_locale(song->genre) : NULL;
		else if (strncmp("%shortfile%", p, n) == 0) {
			if( strstr(song->file, "://") )
				temp = utf8_to_locale(song->file);
			else
				temp = utf8_to_locale(basename(song->file));
		} else if (strncmp("%time%", p, n) == 0) {
			if (song->time != MPD_SONG_NO_TIME)  {
				if (song->time > 3600) {
					temp = g_strdup_printf("%d:%02d:%02d",
							       song->time / 3600,
							       (song->time % 3600) / 60,
							       song->time % 60);
				} else {
					temp = g_strdup_printf("%d:%02d",
							       song->time / 60,
							       song->time % 60);
				}
			}
		}

		if( temp == NULL) {
			gsize templen=n;
			/* just pass-through any unknown specifiers (including esc) */
			/* drop a null char in so printf stops at the end of this specifier,
			   but put the real character back in (pseudo-const) */
			if( length+templen > max )
				templen = max-length;
			g_strlcat(s, p,max);
			length+=templen;
		} else {
			gsize templen = strlen(temp);

			found = TRUE;
			if( length+templen > max )
				templen = max-length;
			g_strlcat(s, temp, max);
			length+=templen;
			g_free(temp);
		}

		/* advance past the specifier */
		p += n;
	}

	if(last) *last = p;

	return length;
}

gsize
strfsong(gchar *s, gsize max, const gchar *format,
	 const struct mpd_song *song)
{
  return _strfsong(s, max, format, song, NULL);
}
     
