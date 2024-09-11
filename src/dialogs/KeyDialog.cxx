// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "KeyDialog.hxx"
#include "ui/Keys.hxx"
#include "ui/Options.hxx"
#include "ui/Window.hxx"
#include "Styles.hxx"

using std::string_view_literals::operator""sv;

void
KeyDialog::OnLeave(const Window window) noexcept
{
	curs_set(0);

	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::STATUS);
}

void
KeyDialog::OnCancel() noexcept
{
	SetResult(-1);
}

void
KeyDialog::Paint(const Window window) const noexcept
{
	if (ui_options.enable_colors)
		window.SetBackgroundStyle(Style::INPUT);

	SelectStyle(window, Style::STATUS_ALERT);
	window.String({0, 0}, prompt);
	window.String(": "sv);

	SelectStyle(window, Style::INPUT);
	window.ClearToEol();

	curs_set(1);
}

static constexpr bool
IsCancelKey(int key) noexcept
{
	return key == KEY_CANCEL || key == KEY_SCANCEL ||
	       key == KEY_CLOSE ||
	       key == KEY_CTL('C') ||
	       key == KEY_CTL('G') ||
	       key == KEY_ESCAPE;
}

bool
KeyDialog::OnKey(Window, int key)
{
	if (IsCancelKey(key))
		Cancel();
	else
		SetResult(key);

	return true;
}
