// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TabBar.hxx"
#include "PageMeta.hxx"
#include "screen_list.hxx"
#include "Styles.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "i18n.h"

static void
PaintPageTab(WINDOW *w, Command cmd, const char *label, bool selected) noexcept
{
	SelectStyle(w, selected ? Style::TITLE : Style::TITLE_BOLD);
	if (selected)
		wattron(w, A_REVERSE);

	waddch(w, ' ');

	const char *key = GetGlobalKeyBindings().GetFirstKeyName(cmd);
	if (key != nullptr)
		waddstr(w, key);

	SelectStyle(w, Style::TITLE);
	if (selected)
		wattron(w, A_REVERSE);

	waddch(w, ':');
	waddstr(w, label);
	waddch(w, ' ');

	if (selected)
		wattroff(w, A_REVERSE);
}

void
PaintTabBar(WINDOW *w, const PageMeta &current_page_meta,
	    const char *current_page_title) noexcept
{
	for (unsigned i = 0;; ++i) {
		const auto *page = GetPageMeta(i);
		if (page == nullptr)
			break;

		const char *title = nullptr;
		if (page == &current_page_meta)
			title = current_page_title;

		if (title == nullptr)
			title = my_gettext(page->title);

		PaintPageTab(w, page->command, title,
			     page == &current_page_meta);
	}
}
