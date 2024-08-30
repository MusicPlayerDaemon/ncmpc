// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TabBar.hxx"
#include "PageMeta.hxx"
#include "screen_list.hxx"
#include "Styles.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "i18n.h"
#include "Window.hxx"

static void
PaintPageTab(const Window window, Command cmd, const char *label, bool selected) noexcept
{
	SelectStyle(window.w, selected ? Style::TITLE : Style::TITLE_BOLD);
	if (selected)
		window.AttributeOn(A_REVERSE);

	window.Char(' ');

	const char *key = GetGlobalKeyBindings().GetFirstKeyName(cmd);
	if (key != nullptr)
		window.String(key);

	SelectStyle(window.w, Style::TITLE);
	if (selected)
		window.AttributeOn(A_REVERSE);

	window.Char(':');
	window.String(label);
	window.Char(' ');

	if (selected)
		window.AttributeOff(A_REVERSE);
}

void
PaintTabBar(const Window window, const PageMeta &current_page_meta,
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

		PaintPageTab(window, page->command, title,
			     page == &current_page_meta);
	}
}
