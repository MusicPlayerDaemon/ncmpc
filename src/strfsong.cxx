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

#include "strfsong.hxx"
#include "charset.hxx"
#include "time_format.hxx"

#include <mpd/client.h>

#include <glib.h>

#include <string.h>

#define MAX_ADDITIONAL_TAGS 8

static const char *
skip(const char * p)
{
	gint stack = 0;

	while (*p != '\0') {
		if (*p == '[')
			stack++;
		if (*p == '#' && p[1] != '\0') {
			/* skip escaped stuff */
			++p;
		} else if (stack) {
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

#ifndef NCMPC_MINI

static char *
concat_tag_values(const char *a, const char *b)
{
	return g_strconcat(a, ", ", b, nullptr);
}

static char *
song_more_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
		     const char *first)
{
	const char *tags[MAX_ADDITIONAL_TAGS];
	unsigned tag_count = 1;
	const char *p;

	tags[0] = first;
	for (tag_count = 1; tag_count < MAX_ADDITIONAL_TAGS && (p =
	     mpd_song_get_tag(song, tag, tag_count)) != nullptr; ++tag_count) {
		tags[tag_count] = p;
	}

	for (int i = (signed)tag_count - 1; i >= 0; i--) {
		for (int j = i - 1; j >= 0; j--) {
			if (strcmp(tags[i], tags[j]) == 0) {
				tags[i] = tags[tag_count - 1];
				tag_count--;
			}
		}
	}

	if (tag_count == 1)
		return nullptr;
	char *buffer = concat_tag_values(tags[0], tags[1]);
	for (unsigned i = 1; i + 1 < tag_count; i++) {
		char *prev = buffer;
		buffer = concat_tag_values(buffer, tags[i]);
		g_free(prev);
	}

	return buffer;
}

#endif /* !NCMPC_MINI */

static char *
song_tag_locale(const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *value = mpd_song_get_tag(song, tag, 0);
	if (value == nullptr)
		return nullptr;

#ifndef NCMPC_MINI
	char *all = song_more_tag_values(song, tag, value);
	if (all != nullptr)
		value = all;
#endif /* !NCMPC_MINI */

	char *result = utf8_to_locale(value);

#ifndef NCMPC_MINI
	g_free(all);
#endif /* !NCMPC_MINI */

	return result;
}

static size_t
_strfsong(char *s,
	  size_t max,
	  const char *format,
	  const struct mpd_song *song,
	  const char **last)
{
	bool found = false;
	/* "missed" helps handling the case of mere literal text like
	   found==true instead of found==false. */
	bool missed = false;

	s[0] = '\0';

	if (song == nullptr)
		return 0;

	const char *p;
	size_t length = 0;
	for (p = format; *p != '\0' && length<max;) {
		/* OR */
		if (p[0] == '|') {
			++p;
			if(missed && !found) {
				s[0] = '\0';
				length = 0;
				missed = false;
			} else {
				p = skip(p);
			}
			continue;
		}

		/* AND */
		if (p[0] == '&') {
			++p;
			if(missed && !found) {
				p = skip(p);
			} else {
				found = false;
				missed = false;
			}
			continue;
		}

		/* EXPRESSION START */
		if (p[0] == '[') {
			char *temp = (char *)g_malloc0(max);
			if( _strfsong(temp, max, p+1, song, &p) >0 ) {
				g_strlcat(s, temp, max);
				length = strlen(s);
				found = true;
			} else {
				missed = true;
			}
			g_free(temp);
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']') {
			if(last) *last = p+1;
			if(missed && !found && length) {
				s[0] = '\0';
				length = 0;
			}
			return length;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '#' && p[0] != '%' && length<max) {
			s[length++] = *p;
			s[length] = '\0';
			p++;
			continue;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0' && length<max) {
			s[length++] = *(p+1);
			s[length] = '\0';
			p+=2;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		char *temp = nullptr;
		const char *end = p + 1;
		while(*end >= 'a' && *end <= 'z') {
			end++;
		}
		size_t n = end - p + 1;
		if(*end != '%')
			n--;
		else if (strncmp("%file%", p, n) == 0)
			temp = utf8_to_locale(mpd_song_get_uri(song));
		else if (strncmp("%artist%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_ARTIST);
			if (temp == nullptr) {
				temp = song_tag_locale(song, MPD_TAG_PERFORMER);
				if (temp == nullptr)
					temp = song_tag_locale(song, MPD_TAG_COMPOSER);
			}
		} else if (strncmp("%albumartist", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_ALBUM_ARTIST);
		else if (strncmp("%composer%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_COMPOSER);
		else if (strncmp("%performer%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_PERFORMER);
		else if (strncmp("%title%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_TITLE);
			if (temp == nullptr)
				temp = song_tag_locale(song, MPD_TAG_NAME);
		} else if (strncmp("%album%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_ALBUM);
		else if (strncmp("%shortalbum%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_ALBUM);
			if (temp) {
				char *temp2 = g_strndup(temp, 25);
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
			temp = song_tag_locale(song, MPD_TAG_TRACK);
		else if (strncmp("%disc%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_DISC);
		else if (strncmp("%name%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_NAME);
		else if (strncmp("%date%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_DATE);
		else if (strncmp("%genre%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_GENRE);
		else if (strncmp("%shortfile%", p, n) == 0) {
			const char *uri = mpd_song_get_uri(song);
			if (strstr(uri, "://") == nullptr)
				uri = g_basename(uri);
			temp = utf8_to_locale(uri);
		} else if (strncmp("%time%", p, n) == 0) {
			unsigned duration = mpd_song_get_duration(song);

			if (duration > 0)  {
				char buffer[32];
				format_duration_short(buffer, sizeof(buffer),
						      duration);
				temp = g_strdup(buffer);
			}
		}

		if( temp == nullptr) {
			size_t templen=n;
			/* just pass-through any unknown specifiers (including esc) */
			if( length+templen > max )
				templen = max-length;
			char *ident = g_strndup(p, templen);
			g_strlcat(s, ident, max);
			length+=templen;
			g_free(ident);

			missed = true;
		} else {
			size_t templen = strlen(temp);

			found = true;
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

size_t
strfsong(char *s, size_t max, const char *format,
	 const struct mpd_song *song)
{
	return _strfsong(s, max, format, song, nullptr);
}

