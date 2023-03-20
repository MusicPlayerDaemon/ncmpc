// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "BasicMarquee.hxx"
#include "util/LocaleString.hxx"

#include <assert.h>
#include <string.h>

std::pair<const char *, size_t>
BasicMarquee::ScrollString() const noexcept
{
	assert(separator != nullptr);

	const char *p = AtCharMB(buffer.data(), buffer.length(), offset);
	const char *end = AtWidthMB(p, strlen(p), width);
	return std::make_pair(p, size_t(end - p));
}

bool
BasicMarquee::Set(unsigned _width, const char *_text) noexcept
{
	assert(separator != nullptr);
	assert(_text != nullptr);

	if (_width == width && text == _text)
		/* no change, do nothing (and, most importantly, do
		   not reset the current offset!) */
		return false;

	width = _width;
	offset = 0;

	text = _text;

	/* create a buffer containing the string and the separator */
	buffer = text + separator + text + separator;
	max_offset = StringLengthMB(buffer.data(), buffer.length() / 2);

	return true;
}

void
BasicMarquee::Clear() noexcept
{
	width = 0;
	text.clear();
	buffer.clear();
}
