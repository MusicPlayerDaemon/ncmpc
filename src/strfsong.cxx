// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "strfsong.hxx"
#include "charset.hxx"
#include "time_format.hxx"
#include "util/CharUtil.hxx"
#include "util/UriUtil.hxx"
#include "TagMask.hxx"

#include <mpd/client.h>

#include <algorithm>

#include <string.h>

using std::string_view_literals::operator""sv;

[[gnu::pure]]
static const char *
skip(const char *p) noexcept
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
	   std::string_view src) noexcept
{
	if (src.size() >= size_t(dest_end - dest))
		src = src.substr(0, dest_end - dest - 1);

	return std::copy(src.begin(), src.end(), dest);
}

static char *
CopyStringFromUTF8(char *dest, char *const dest_end,
		   std::string_view src_utf8) noexcept
{
	return CopyUtf8ToLocale({dest, dest_end}, src_utf8);
}

static char *
CopyTag(char *dest, char *const end,
	const struct mpd_song &song, enum mpd_tag_type tag) noexcept
{
	const char *value = mpd_song_get_tag(&song, tag, 0);
	if (value == nullptr)
		return dest;

	dest = CopyStringFromUTF8(dest, end, value);

	for (unsigned i = 1; dest + 5 < end &&
		     (value = mpd_song_get_tag(&song, tag, i)) != nullptr;
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
	  const struct mpd_song &song,
	  const char **last) noexcept
{
	bool found = false;
	/* "missed" helps handling the case of mere literal text like
	   found==true instead of found==false. */
	bool missed = false;

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
		while (IsLowerAlphaASCII(*name_end))
			++name_end;
		const std::string_view name{p + 1, name_end};
		size_t n = name_end - p + 1;

		std::string_view value{}, value_utf8{};
		enum mpd_tag_type tag = MPD_TAG_UNKNOWN;
		bool short_tag = false;
		char buffer[32];

		if (*name_end != '%')
			n--;
		else if (name == "file"sv)
			value_utf8 = mpd_song_get_uri(&song);
		else if (name == "artist"sv)
			tag = MPD_TAG_ARTIST;
		else if (name == "albumartist"sv)
			tag = MPD_TAG_ALBUM_ARTIST;
		else if (name == "composer"sv)
			tag = MPD_TAG_COMPOSER;
		else if (name == "performer"sv)
			tag = MPD_TAG_PERFORMER;
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
		else if (name == "conductor"sv)
			tag = MPD_TAG_CONDUCTOR;
		else if (name == "work"sv)
			tag = MPD_TAG_WORK;
		else if (name == "grouping"sv)
			tag = MPD_TAG_GROUPING;
		else if (strncmp("%label%", p, n) == 0)
			tag = MPD_TAG_LABEL;
#endif
		else if (name == "title"sv)
			tag = MPD_TAG_TITLE;
		else if (name == "album"sv)
			tag = MPD_TAG_ALBUM;
		else if (name == "shortalbum"sv) {
			tag = MPD_TAG_ALBUM;
			short_tag = true;
		}
		else if (name == "track"sv)
			tag = MPD_TAG_TRACK;
		else if (name == "disc"sv)
			tag = MPD_TAG_DISC;
		else if (name == "name"sv)
			tag = MPD_TAG_NAME;
		else if (name == "date"sv)
			tag = MPD_TAG_DATE;
		else if (name == "genre"sv)
			tag = MPD_TAG_GENRE;
		else if (name == "shortfile"sv) {
			const char *uri = mpd_song_get_uri(&song);
			if (strstr(uri, "://") == nullptr)
				uri = GetUriFilename(uri);
			value_utf8 = uri;
		} else if (name == "time"sv) {
			unsigned duration = mpd_song_get_duration(&song);

			if (duration > 0)  {
				value = format_duration_short(buffer, duration);
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

		if (value_utf8.data() != nullptr) {
			found = true;
			s = CopyStringFromUTF8(s, end, value_utf8);
			continue;
		}

		if (value.data() == nullptr) {
			/* just pass-through any unknown specifiers (including esc) */
			value = p;

			missed = true;
		} else {
			found = true;
		}

		s = CopyString(s, end, value);
	}

	if(last) *last = p;

	*s = '\0';
	return s - s0;
}

std::string_view
strfsong(std::span<char> buffer, const char *format,
	 const struct mpd_song &song) noexcept
{
	std::size_t length = _strfsong(buffer.data(), buffer.data() + buffer.size(), format, song, nullptr);
	return {buffer.data(), length};
}

TagMask
SongFormatToTagMask(const char *format) noexcept
{
	TagMask mask = TagMask::None();

	/* TODO: this is incomplete and not correct; the missing tags
	   are already in global_tag_whitelist (see Instance.cxx); but
	   for now, this implementation may be good enough */

	static constexpr struct {
		const char *s;
		enum mpd_tag_type tag;
	} tag_references[] = {
		{"%albumartist%", MPD_TAG_ALBUM_ARTIST},
		{"%composer%", MPD_TAG_COMPOSER},
		{"%performer%", MPD_TAG_PERFORMER},
	};

	for (const auto &i : tag_references)
		if (strstr(format, i.s) != nullptr)
			mask |= i.tag;

	return mask;
}
