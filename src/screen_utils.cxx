// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_utils.hxx"
#include "screen.hxx"
#include "Styles.hxx"

using std::string_view_literals::operator""sv;

static std::string_view
CompletionDisplayString(std::string_view value) noexcept
{
	if (value.ends_with('/')) {
		/* if the string ends with a slash (directory URIs
		   usually do), backtrack to the preceding slash (if
		   any) */

		if (value.size() > 2) {
			if (const auto slash = value.rfind('/', value.size() - 2);
			    slash != value.npos)
				value = value.substr(slash + 1);
		}

		return value;
	} else {
		if (const auto slash = value.rfind('/');
		    slash != value.npos)
			value = value.substr(slash);

		return value;
	}
}

void
screen_display_completion_list(ScreenManager &screen, Completion::Range range) noexcept
{
	static Completion::Range prev_range;
	static size_t prev_length = 0;
	static unsigned offset = 0;
	const Window window = screen.main_window;
	const unsigned height = screen.main_window.GetHeight();

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

	SelectStyle(window, Style::STATUS_ALERT);

	auto i = std::next(range.begin(), offset);
	for (unsigned y = 0; y < height; ++y, ++i) {
		window.MoveCursor({0, (int)y});
		if (i == range.end())
			break;

		window.String(CompletionDisplayString(*i));
		window.ClearToEol();
	}

	window.ClearToBottom();

	window.Refresh();
	SelectStyle(window, Style::LIST);
}
