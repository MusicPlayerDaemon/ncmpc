// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TabBar.hxx"
#include "PageMeta.hxx"
#include "AllPages.hxx"
#include "Styles.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "i18n.h"
#include "ui/Window.hxx"
#include "util/LocaleString.hxx"

static void
PaintPageTab(const Window window, Command cmd, std::string_view label, bool selected,
	     const KeyBindings &key_bindings) noexcept
{
	SelectStyle(window, selected ? Style::TITLE : Style::TITLE_BOLD);
	if (selected)
		window.AttributeOn(A_REVERSE);

	window.Char(' ');

	const char *key = key_bindings.GetFirstKeyName(cmd);
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
TabBar::Paint(const Window window, const PageMeta &current_page_meta,
	      std::string_view current_page_title) const noexcept
{
	const auto &key_bindings = GetGlobalKeyBindings();

	tabs.clear();

	unsigned x = 0;
	for (const PageMeta *const*i = all_pages; *i != nullptr && !tabs.full(); ++i) {
		const auto &page = **i;

		std::string_view title{};
		if (&page == &current_page_meta)
			title = current_page_title;

		if (title.data() == nullptr)
			title = my_gettext(page.title);

		PaintPageTab(window, page.command, title,
			     &page == &current_page_meta,
			     key_bindings);

		unsigned new_x = window.GetCursor().x;
		unsigned width = new_x - x;
		x = new_x;

		tabs.emplace_back(page, width);
	}
}

const PageMeta *
TabBar::GetTabAtX(unsigned x) const noexcept
{
	for (const auto &i : tabs) {
		if (x < i.width)
			return &i.meta;

		x -= i.width;
	}

	return nullptr;
}
