/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#include "StringUTF8.hxx"

#include <string.h>

#ifdef HAVE_LOCALE_T
#include <langinfo.h>
#include <locale.h>

static locale_t utf8_locale = locale_t(0);

ScopeInitUTF8::ScopeInitUTF8() noexcept
{
	const char *charset = nl_langinfo(CODESET);
	if (charset == nullptr || strcasecmp(charset, "utf-8") == 0)
		/* if we're already UTF-8, we don't need a special
		   UTF-8 locale */
		return;

	locale_t l = duplocale(LC_GLOBAL_LOCALE);
	if (l == locale_t(0))
		return;

	locale_t l2 = newlocale(LC_COLLATE_MASK, "en_US.UTF-8", l);
	if (l2 == locale_t(0)) {
		freelocale(l);
		return;
	}

	utf8_locale = l2;
}

ScopeInitUTF8::~ScopeInitUTF8() noexcept
{
	if (utf8_locale != locale_t(0)) {
		freelocale(utf8_locale);
		utf8_locale = locale_t(0);
	}
}

#endif

gcc_pure
int
CollateUTF8(const char *a, const char *b)
{
#ifdef HAVE_LOCALE_T
	if (utf8_locale != locale_t(0))
		return strcoll_l(a, b, utf8_locale);
#endif

	return strcoll(a, b);
}
