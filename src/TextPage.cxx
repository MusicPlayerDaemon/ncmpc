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

#include "TextPage.hxx"
#include "TextListRenderer.hxx"
#include "screen_find.hxx"
#include "charset.hxx"

#include <glib.h>

#include <algorithm>

#include <assert.h>
#include <string.h>

void
TextPage::Clear()
{
	lw.Reset();
	lines.clear();
	lw.SetLength(0);
}

void
TextPage::Append(const char *str)
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
TextPage::GetListItemText(unsigned idx) const
{
	assert(idx < lines.size());

	static char buffer[256];
	g_strlcpy(buffer, Utf8ToLocale(lines[idx].c_str()).c_str(),
		  sizeof(buffer));
	return buffer;
}

void
TextPage::Paint() const
{
	lw.Paint(TextListRenderer(*this));
}

bool
TextPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	lw.SetCursor(lw.start);
	if (screen_find(screen, &lw, cmd, *this)) {
		/* center the row */
		lw.Center(lw.selected);
		SetDirty();
		return true;
	}

	return false;
}
