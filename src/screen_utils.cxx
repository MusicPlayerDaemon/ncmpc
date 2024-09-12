// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_utils.hxx"
#include "screen.hxx"
#include "Styles.hxx"

using std::string_view_literals::operator""sv;

void
screen_display_completion_list(ScreenManager &screen,
			       std::string_view prefix,
			       Completion::Range range) noexcept
{
	static Completion::Range prev_range;
	static size_t prev_length = 0;
	static unsigned offset = 0;
	const Window window = screen.main_window;
	const unsigned height = screen.main_window.GetHeight();

	if (!prefix.ends_with('/')) {
		/* the current prefix is not a directory; backtrack to
		   the last slash because we want to strip only
		   directories and not truncate filenames */

		if (const auto slash = prefix.rfind('/'); slash != prefix.npos) {
			prefix = prefix.substr(0, slash + 1);
		} else {
			prefix = {};
		}
	}

	size_t length = std::distance(range.begin(), range.end());
	if (range == prev_range && length == prev_length) {
		offset += height;
		if (offset >= length)
			offset = 0;
	} else {
		prev_range = range;
		prev_length = length;
		offset = 0;
	}

	SelectStyle(window, Style::LIST);

	auto i = std::next(range.begin(), offset);
	for (unsigned y = 0; y < height; ++i) {
		window.MoveCursor({0, (int)y});
		if (i == range.end())
			break;

		std::string_view s = *i;
		assert(s.size() >= prefix.size());

		if (s.size() > prefix.size())
			/* strip the prefix */
			s = s.substr(prefix.size());
		else
			/* if the prefix equals this item, use this
                           special string which means "current
                           directory" */
			s = "."sv;

		if (const auto slash = s.find('/');
		    slash != s.npos && slash + 1 < s.size())
			/* this item is from a deeper directory level:
			   skip it */
			continue;

		window.String(s);
		window.ClearToEol();
		++y;
	}

	window.ClearToBottom();

	window.Refresh();
}
