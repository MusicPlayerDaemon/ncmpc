// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_utils.hxx"
#include "screen.hxx"
#include "config.h"
#include "i18n.h"
#include "Options.hxx"
#include "Styles.hxx"
#include "wreadln.hxx"
#include "config.h"

#ifndef _WIN32
#include "WaitUserInput.hxx"
#include <cerrno>
#endif

#include <string.h>

void
screen_bell() noexcept
{
	if (options.audible_bell)
		beep();
	if (options.visible_bell)
		flash();
}

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
	WINDOW *w = screen.status_bar.GetWindow().w;

	SelectStyle(w, Style::STATUS_ALERT);
	werase(w);
	wmove(w, 0, 0);
	waddstr(w, prompt);

	echo();
	curs_set(1);

#ifndef _WIN32
	WaitUserInput wui;
#endif

	int key;
	do {
		key = wgetch(w);

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
	WINDOW *w = window.w;

	wmove(w, 0,0);
	curs_set(1);

	if (prompt != nullptr) {
		SelectStyle(w, Style::STATUS_ALERT);
		waddstr(w, prompt);
		waddstr(w, ": ");
	}

	SelectStyle(w, Style::STATUS);
	wattron(w, A_REVERSE);

	auto result = wreadln(w, value, window.GetWidth(),
			      history, completion);
	curs_set(0);
	return result;
}

std::string
screen_read_password(ScreenManager &screen, const char *prompt) noexcept
{
	const auto &window = screen.status_bar.GetWindow();
	WINDOW *w = window.w;

	wmove(w, 0,0);
	curs_set(1);
	SelectStyle(w, Style::STATUS_ALERT);

	if (prompt == nullptr)
		prompt = _("Password");

	waddstr(w, prompt);
	waddstr(w, ": ");

	SelectStyle(w, Style::STATUS);
	wattron(w, A_REVERSE);

	auto result = wreadln_masked(w, nullptr, window.GetWidth());
	curs_set(0);
	return result;
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
	WINDOW *w = screen.main_window.w;
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

	SelectStyle(w, Style::STATUS_ALERT);

	auto i = std::next(range.begin(), offset);
	for (unsigned y = 0; y < height; ++y, ++i) {
		wmove(w, y, 0);
		if (i == range.end())
			break;

		const char *value = i->c_str();
		waddstr(w, CompletionDisplayString(value));
		wclrtoeol(w);
	}

	wclrtobot(w);

	wrefresh(w);
	SelectStyle(w, Style::LIST);
}
