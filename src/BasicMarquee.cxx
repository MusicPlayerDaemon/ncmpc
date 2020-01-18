/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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
