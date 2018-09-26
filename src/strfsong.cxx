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
#include "util/UriUtil.hxx"

#include <mpd/client.h>

#include <algorithm>

#include <assert.h>
#include <string.h>

static const char *
skip(const char * p)
{
	unsigned stack = 0;

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

static char *
CopyString(char *dest, char *const dest_end,
	   const char *src, size_t length) noexcept
{
	if (length >= size_t(dest_end - dest))
		length = dest_end - dest - 1;

	return std::copy_n(src, length, dest);
}

static char *
CopyStringFromUTF8(char *dest, char *const dest_end,
		   const char *src_utf8) noexcept
{
	return CopyUtf8ToLocale(dest, dest_end - dest, src_utf8);
}

static char *
CopyTag(char *dest, char *const end,
	const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *value = mpd_song_get_tag(song, tag, 0);
	if (value == nullptr)
		return dest;

	dest = CopyStringFromUTF8(dest, end, value);

	for (unsigned i = 1; dest + 5 < end &&
		     (value = mpd_song_get_tag(song, tag, i)) != nullptr;
	     ++i) {
		*dest++ = ',';
		*dest++ = ' ';
		dest = CopyStringFromUTF8(dest, end, value);
	}

	return dest;
}

static size_t
_strfsong(char *const s0, char *const end,
	  const char *format,
	  const struct mpd_song *song,
	  const char **last)
{
	bool found = false;
	/* "missed" helps handling the case of mere literal text like
	   found==true instead of found==false. */
	bool missed = false;


	if (song == nullptr) {
		s0[0] = '\0';
		return 0;
	}

	char *s = s0;
	const char *p;
	for (p = format; *p != '\0' && s < end - 1;) {
		/* OR */
		if (p[0] == '|') {
			++p;
			if(missed && !found) {
				s = s0;
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
			size_t n = _strfsong(s, end, p + 1,
					     song, &p);
			if (n > 0) {
				s += n;
				found = true;
			} else {
				missed = true;
			}
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']') {
			++p;
			if (missed && !found)
				s = s0;
			break;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0') {
			*s++ = p[1];
			p+=2;
			continue;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '%') {
			*s++ = *p++;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		const char *name_end = p + 1;
		while (*name_end >= 'a' && *name_end <= 'z')
			++name_end;
		size_t n = name_end - p + 1;

		const char *value = nullptr, *value_utf8 = nullptr;
		enum mpd_tag_type tag = MPD_TAG_UNKNOWN;
		bool short_tag = false;
		char buffer[32];

		if (*name_end != '%')
			n--;
		else if (strncmp("%file%", p, n) == 0)
			value_utf8 = mpd_song_get_uri(song);
		else if (strncmp("%artist%", p, n) == 0)
			tag = MPD_TAG_ARTIST;
		else if (strncmp("%albumartist%", p, n) == 0)
			tag = MPD_TAG_ALBUM_ARTIST;
		else if (strncmp("%composer%", p, n) == 0)
			tag = MPD_TAG_COMPOSER;
		else if (strncmp("%performer%", p, n) == 0)
			tag = MPD_TAG_PERFORMER;
		else if (strncmp("%title%", p, n) == 0)
			tag = MPD_TAG_TITLE;
		else if (strncmp("%album%", p, n) == 0)
			tag = MPD_TAG_ALBUM;
		else if (strncmp("%shortalbum%", p, n) == 0) {
			tag = MPD_TAG_ALBUM;
			short_tag = true;
		}
		else if (strncmp("%track%", p, n) == 0)
			tag = MPD_TAG_TRACK;
		else if (strncmp("%disc%", p, n) == 0)
			tag = MPD_TAG_DISC;
		else if (strncmp("%name%", p, n) == 0)
			tag = MPD_TAG_NAME;
		else if (strncmp("%date%", p, n) == 0)
			tag = MPD_TAG_DATE;
		else if (strncmp("%genre%", p, n) == 0)
			tag = MPD_TAG_GENRE;
		else if (strncmp("%shortfile%", p, n) == 0) {
			const char *uri = mpd_song_get_uri(song);
			if (strstr(uri, "://") == nullptr)
				uri = GetUriFilename(uri);
			value_utf8 = uri;
		} else if (strncmp("%time%", p, n) == 0) {
			unsigned duration = mpd_song_get_duration(song);

			if (duration > 0)  {
				format_duration_short(buffer, sizeof(buffer),
						      duration);
				value = buffer;
			}
		}

		/* advance past the specifier */
		p += n;

		if (tag != MPD_TAG_UNKNOWN) {
			char *const old = s;
			s = CopyTag(s, end, song, tag);
			if (s != old) {
				found = true;

				if (short_tag && s > old + 25)
					s = std::copy_n("...", 3, old + 22);
			} else
				missed = true;

			continue;
		}

		if (value_utf8 != nullptr) {
			found = true;
			s = CopyStringFromUTF8(s, end, value_utf8);
			continue;
		}

		size_t value_length;

		if (value == nullptr) {
			/* just pass-through any unknown specifiers (including esc) */
			value = p;
			value_length = n;

			missed = true;
		} else {
			value_length = strlen(value);

			found = true;
		}

		s = CopyString(s, end, value, value_length);
	}

	if(last) *last = p;

	*s = '\0';
	return s - s0;
}

size_t
strfsong(char *s, size_t max, const char *format,
	 const struct mpd_song *song)
{
	return _strfsong(s, s + max, format, song, nullptr);
}
