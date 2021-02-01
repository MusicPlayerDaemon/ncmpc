/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "TextPage.hxx"
#include "TextListRenderer.hxx"
#include "screen_find.hxx"
#include "charset.hxx"

#include <algorithm>

#include <assert.h>
#include <string.h>

void
TextPage::Clear() noexcept
{
	lw.Reset();
	lines.clear();
	lw.SetLength(0);
}

void
TextPage::Append(const char *str) noexcept
{
	assert(str != nullptr);

	const char *eol;
	while ((eol = strchr(str, '\n')) != nullptr) {
		const char *next = eol + 1;

		/* strip whitespace at end */

		while (eol > str && (unsigned char)eol[-1] <= 0x20)
			--eol;

		/* create copy and append it to lines */

		lines.emplace_back(str, eol);

		/* reset control characters */

		std::replace_if(lines.back().begin(), lines.back().end(),
				[](unsigned char ch){
					return ch < 0x20;
				}, ' ');

		str = next;
	}

	if (*str != 0)
		lines.emplace_back(str);

	lw.SetLength(lines.size());
}

const char *
TextPage::GetListItemText(char *buffer, size_t size, unsigned idx) const noexcept
{
	assert(idx < lines.size());

	return utf8_to_locale(lines[idx].c_str(), buffer, size);
}

void
TextPage::Paint() const noexcept
{
	lw.Paint(TextListRenderer(*this));
}

bool
TextPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	if (!lw.IsCursorVisible())
		/* start searching at the beginning of the page (not
		   where the invisible cursor just happens to be),
		   unless the cursor is still visible from the last
		   search */
		lw.SetCursorFromOrigin(0);

	if (screen_find(screen, lw, cmd, *this)) {
		SetDirty();
		return true;
	}

	return false;
}
