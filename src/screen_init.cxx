// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen.hxx"
#include "Page.hxx"
#include "QueuePage.hxx"
#include "config.h"
#include "Styles.hxx"
#include "ui/Options.hxx"

/* minimum window size */
static const unsigned SCREEN_MIN_COLS = 14;
static const unsigned SCREEN_MIN_ROWS = 5;

ScreenManager::ScreenManager(EventLoop &event_loop) noexcept
	:paint_event(event_loop, BIND_THIS_METHOD(Paint)),
	 layout({std::max<unsigned>(COLS, SCREEN_MIN_COLS),
		 std::max<unsigned>(LINES, SCREEN_MIN_ROWS)}),
	 title_bar(layout.title, layout.size.width),
	 main_window(layout.main, layout.GetMainSize()),
	 progress_bar(layout.GetProgress(), layout.size.width),
	 status_bar(event_loop, layout.GetStatus(), layout.size.width),
	 mode_fn_prev(&screen_queue)
{
	buf_size = layout.size.width;
	buf = new char[buf_size];

	if (!ui_options.hardware_cursor)
		leaveok(main_window.w, true);

	keypad(main_window.w, true);

#ifdef ENABLE_COLORS
	if (ui_options.enable_colors) {
		/* set background attributes */
		main_window.SetBackgroundStyle(Style::LIST);
	}
#endif
}

ScreenManager::~ScreenManager() noexcept
{
	delete[] buf;
}

void
ScreenManager::Exit() noexcept
{
	current_page->second->OnClose();
	current_page = pages.end();
	pages.clear();
}

void
ScreenManager::OnResize() noexcept
{
	layout = Layout({std::max<unsigned>(COLS, SCREEN_MIN_COLS),
			 std::max<unsigned>(LINES, SCREEN_MIN_ROWS)});

#ifdef PDCURSES
	resize_term(layout.size.height, layout.size.width);
#else
	resizeterm(layout.size.height, layout.size.width);
#endif

	title_bar.OnResize(layout.size.width);

	/* main window */
	main_window.Resize(layout.GetMainSize());

	/* progress window */
	progress_bar.OnResize(layout.GetProgress(), layout.size.width);

	/* status window */
	status_bar.OnResize(layout.GetStatus(), layout.size.width);

	buf_size = layout.size.width;
	delete[] buf;
	buf = new char[buf_size];

	/* resize all screens */
	current_page->second->Resize(main_window.GetSize());

	/* ? - without this the cursor becomes visible with aterm & Eterm */
	curs_set(1);
	curs_set(0);

	SchedulePaint();
}

void
ScreenManager::Init(struct mpdclient *c) noexcept
{
	current_page = MakePage(screen_queue);
	current_page->second->OnOpen(*c);
}
