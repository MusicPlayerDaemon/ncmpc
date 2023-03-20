// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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

[[gnu::pure]]
int
CollateUTF8(const char *a, const char *b)
{
#ifdef HAVE_LOCALE_T
	if (utf8_locale != locale_t(0))
		return strcoll_l(a, b, utf8_locale);
#endif

	return strcoll(a, b);
}
