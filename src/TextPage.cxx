// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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

	return utf8_to_locale(lines[idx].c_str(), {buffer, size});
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
