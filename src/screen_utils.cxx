// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_utils.hxx"
#include "screen.hxx"
#include "config.h"
#include "i18n.h"
#include "Styles.hxx"
#include "wreadln.hxx"
#include "config.h"

#ifndef _WIN32
#include "WaitUserInput.hxx"
#include <cerrno>
#endif

#include <string.h>

using std::string_view_literals::operator""sv;

static constexpr bool
ignore_key(int key) noexcept
{
	return
#ifdef HAVE_GETMOUSE
		/* ignore mouse events */
		key == KEY_MOUSE ||
#endif
		key == ERR;
}

int
screen_getch(ScreenManager &screen, const char *prompt) noexcept
{
	const auto &window = screen.status_bar.GetWindow();

	SelectStyle(window, Style::STATUS_ALERT);
	window.Erase();
	window.String({0, 0}, prompt);

	echo();
	curs_set(1);

#ifndef _WIN32
	WaitUserInput wui;
#endif

	int key;
	do {
		key = window.GetChar();

#ifndef _WIN32
		if (key == ERR && errno == EAGAIN) {
			if (wui.Wait())
				continue;
			else
				break;
		}
#endif
	} while (ignore_key(key));

	noecho();
	curs_set(0);

	return key;
}

bool
screen_get_yesno(ScreenManager &screen, const char *_prompt, bool def) noexcept
{
	/* NOTE: if one day a translator decides to use a multi-byte character
	   for one of the yes/no keys, we'll have to parse it properly */

	char prompt[256];
	snprintf(prompt, sizeof(prompt),
		 "%s [%s/%s] ", _prompt,
		 YES_TRANSLATION, NO_TRANSLATION);
	int key = tolower(screen_getch(screen, prompt));
	if (key == YES_TRANSLATION[0])
		return true;
	else if (key == NO_TRANSLATION[0])
		return false;
	else
		return def;
}

std::string
screen_readln(ScreenManager &screen, const char *prompt,
	      const char *value,
	      History *history,
	      Completion *completion) noexcept
{
	const auto &window = screen.status_bar.GetWindow();

	window.MoveCursor({0, 0});

	if (prompt != nullptr) {
		SelectStyle(window, Style::STATUS_ALERT);
		window.String(prompt);
		window.String(": "sv);
	}

	SelectStyle(window, Style::STATUS);
	window.AttributeOn(A_REVERSE);

	return wreadln(window, value, history, completion);
}

std::string
screen_read_password(ScreenManager &screen, const char *prompt) noexcept
{
	const auto &window = screen.status_bar.GetWindow();

	window.MoveCursor({0, 0});
	SelectStyle(window, Style::STATUS_ALERT);

	if (prompt == nullptr)
		prompt = _("Password");

	window.String(prompt);
	window.String(": "sv);

	SelectStyle(window, Style::STATUS);
	window.AttributeOn(A_REVERSE);

	return wreadln_masked(window, nullptr);
}

static const char *
CompletionDisplayString(const char *value) noexcept
{
	const char *slash = strrchr(value, '/');
	if (slash == nullptr)
		return value;

	if (slash[1] == 0) {
		/* if the string ends with a slash (directory URIs
		   usually do), backtrack to the preceding slash (if
		   any) */
		while (slash > value) {
			--slash;

			if (*slash == '/')
				return slash + 1;
		}
	}

	return slash;
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

		const char *value = i->c_str();
		window.String(CompletionDisplayString(value));
		window.ClearToEol();
	}

	window.ClearToBottom();

	window.Refresh();
	SelectStyle(window, Style::LIST);
}
