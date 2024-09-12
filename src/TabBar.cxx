// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TabBar.hxx"
#include "PageMeta.hxx"
#include "screen_list.hxx"
#include "Styles.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "i18n.h"
#include "ui/Window.hxx"
#include "util/LocaleString.hxx"

static void
PaintPageTab(const Window window, Command cmd, std::string_view label, bool selected) noexcept
{
	SelectStyle(window, selected ? Style::TITLE : Style::TITLE_BOLD);
	if (selected)
		window.AttributeOn(A_REVERSE);

	window.Char(' ');

	const char *key = GetGlobalKeyBindings().GetFirstKeyName(cmd);
	if (key != nullptr)
		window.String(key);

	SelectStyle(window, Style::TITLE);
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
	    std::string_view current_page_title) noexcept
{
	for (unsigned i = 0;; ++i) {
		const auto *page = GetPageMeta(i);
		if (page == nullptr)
			break;

		std::string_view title{};
		if (page == &current_page_meta)
			title = current_page_title;

		if (title.data() == nullptr)
			title = my_gettext(page->title);

		PaintPageTab(window, page->command, title,
			     page == &current_page_meta);
	}
}

static unsigned
GetTabWidth(Command cmd, std::string_view label) noexcept
{
	unsigned width = 3 + StringWidthMB(label);

	const char *key = GetGlobalKeyBindings().GetFirstKeyName(cmd);
	if (key != nullptr)
		width += StringWidthMB(key);

	return width;
}

const PageMeta *
GetTabAtX(const PageMeta &current_page_meta,
	  std::string_view current_page_title,
	  unsigned x) noexcept
{
	for (unsigned i = 0;; ++i) {
		const auto *page = GetPageMeta(i);
		if (page == nullptr)
			break;

		std::string_view title{};
		if (page == &current_page_meta)
			title = current_page_title;

		if (title.data() == nullptr)
			title = my_gettext(page->title);

		const unsigned tab_width = GetTabWidth(page->command, title);
		if (x < tab_width)
			return page;

		x -= tab_width;
	}

	return nullptr;
}
