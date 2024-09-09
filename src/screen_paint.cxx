// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen.hxx"
#include "page/Page.hxx"
#include "ui/Options.hxx"

void
ScreenManager::PaintTopWindow() noexcept
{
	const auto title = GetCurrentPage().GetTitle({buf, buf_size});
	title_bar.Paint(GetCurrentPageMeta(), title);
}

void
ScreenManager::Paint() noexcept
{
	/* update title/header window */
	PaintTopWindow();

	/* paint the bottom window */

	progress_bar.Paint();

	const auto &page = GetCurrentPage();
	if (!page.PaintStatusBarOverride(status_bar.GetWindow()))
		status_bar.Paint();

	/* paint the main window */

	if (main_dirty) {
		main_dirty = false;
		page.Paint();
	}

	/* move the cursor to the origin */

	if (!ui_options.hardware_cursor)
		main_window.MoveCursor({0, 0});

	main_window.RefreshNoOut();

	/* tell curses to update */
	doupdate();
}
