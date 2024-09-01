// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "BasicMarquee.hxx"
#include "util/LocaleString.hxx"

#include <fmt/core.h>

#include <assert.h>
#include <string.h>

using std::string_view_literals::operator""sv;

std::string_view
BasicMarquee::ScrollString() const noexcept
{
	return TruncateAtWidthMB(SplitAtCharMB(buffer, offset).second, width);
}

bool
BasicMarquee::Set(unsigned _width, std::string_view _text) noexcept
{
	if (_width == width && text == _text)
		/* no change, do nothing (and, most importantly, do
		   not reset the current offset!) */
		return false;

	width = _width;
	offset = 0;

	text = _text;

	/* create a buffer containing the string and the separator */
	buffer = fmt::format("{}{}{}{}"sv, text, separator, text, separator);
	max_offset = StringLengthMB(buffer.substr(buffer.size() / 2));

	return true;
}

void
BasicMarquee::Clear() noexcept
{
	width = 0;
	text.clear();
	buffer.clear();
}
