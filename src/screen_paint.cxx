// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen.hxx"
#include "Page.hxx"
#include "Options.hxx"

void
ScreenManager::PaintTopWindow() noexcept
{
	const auto title = current_page->second->GetTitle({buf, buf_size});
	title_bar.Paint(GetCurrentPageMeta(), title);
}

void
ScreenManager::Paint() noexcept
{
	/* update title/header window */
	PaintTopWindow();

	/* paint the bottom window */

	progress_bar.Paint();

	if (!current_page->second->PaintStatusBarOverride(status_bar.GetWindow()))
		status_bar.Paint();

	/* paint the main window */

	if (current_page->second->IsDirty()) {
		current_page->second->Paint();
		current_page->second->SetDirty(false);
	}

	/* move the cursor to the origin */

	if (!options.hardware_cursor)
		main_window.MoveCursor({0, 0});

	main_window.RefreshNoOut();

	/* tell curses to update */
	doupdate();
}
