// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TextPage.hxx"
#include "FindSupport.hxx"
#include "charset.hxx"
#include "ui/TextListRenderer.hxx"
#include "util/CharUtil.hxx"
#include "util/IterableSplitString.hxx"
#include "util/StringStrip.hxx"

#include <algorithm>

#include <assert.h>

TextPage::TextPage(PageContainer &_parent, Window window,
		   FindSupport &_find_support) noexcept
	:ListPage(_parent, window), find_support(_find_support)
{
	lw.HideCursor();
}

void
TextPage::Clear() noexcept
{
	lw.Reset();
	lines.clear();
	lw.SetLength(0);
}

void
TextPage::Append(std::string_view v) noexcept
{
	for (std::string_view line : IterableSplitString(v, '\n')) {
		/* strip whitespace at end */

		line = StripRight(line);

		/* create copy and append it to lines */

		lines.emplace_back(line);

		/* reset control characters */

		std::replace_if(lines.back().begin(), lines.back().end(),
				IsNonPrintableASCII, ' ');
	}

	lw.SetLength(lines.size());

	SchedulePaint();
}

std::string_view
TextPage::GetListItemText(std::span<char> buffer, unsigned idx) const noexcept
{
	assert(idx < lines.size());

	return utf8_to_locale(lines[idx], buffer);
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

	if (auto task = find_support.Find(lw, *this, cmd)) {
		CoStart(std::move(task));
		return true;
	}

	return false;
}
