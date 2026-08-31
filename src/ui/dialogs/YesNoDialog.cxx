// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "YesNoDialog.hxx"
#include "ui/Bell.hxx"
#include "ui/Keys.hxx"
#include "ui/Options.hxx"
#include "ui/Window.hxx"
#include "Styles.hxx"
#include "i18n.h"

#include <ctype.h>

using std::string_view_literals::operator""sv;

void
YesNoDialog::OnLeave(const Window window) noexcept
{
	noecho();
	curs_set(0);

	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::STATUS);
}

void
YesNoDialog::OnCancel() noexcept
{
	SetResult(YesNoResult::CANCEL);
}

void
YesNoDialog::Paint(const Window window) const noexcept
{
	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::INPUT);

	SelectStyle(window, Style::STATUS_ALERT);
	window.String({0, 0}, prompt);
	window.String(" ["sv);
	window.String(YES_TRANSLATION);
	window.Char('/');
	window.String(NO_TRANSLATION);
	window.String("] "sv);

	SelectStyle(window, Style::INPUT);
	window.ClearToEol();

	echo();
	curs_set(1);
}

static constexpr bool
IsCancelKey(int key) noexcept
{
	return key == KEY_CANCEL || key == KEY_SCANCEL ||
	       key == KEY_CLOSE ||
	       key == KEY_UNDO ||
	       key == KEY_CTL('C') ||
	       key == KEY_CTL('G') ||
	       key == KEY_ESCAPE;
}

bool
YesNoDialog::OnKey(Window, int key)
{
	/* NOTE: if one day a translator decides to use a multi-byte character
	   for one of the yes/no keys, we'll have to parse it properly */

	key = tolower(key);
	if (key == YES_TRANSLATION[0]) {
		Hide();
		SetResult(YesNoResult::YES);
	} else if (key == NO_TRANSLATION[0]) {
		Hide();
		SetResult(YesNoResult::NO);
	} else if (IsCancelKey(key)) {
		Cancel();
	} else {
		Bell();
	}

	return true;
}
