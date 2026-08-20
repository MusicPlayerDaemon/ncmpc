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

	/* collect all pages with hot keys F1..F10 or 1..9,0; these
	   are the "main" pages that shall be displayed always */
	std::array<const PageMeta *, 10> n_pages{}, f_pages{};
	for (const PageMeta *const*i = all_pages; *i != nullptr; ++i) {
		const auto &page = **i;
		const auto &key_binding = key_bindings.Get(page.command);
		for (auto key : key_binding.keys) {
			if (key >= '1' && key <= '9')
				n_pages[key - '1'] = &page;
			else if (key == '0')
				n_pages.back() = &page;
			else if (key >= KEY_F(1) && key <= KEY_F(10))
				n_pages[key - KEY_F(1)] = &page;
		}
	}

	/* combine the sparse arrays into a single array and append
	   the current page (unless it's already visible */
	StaticVector<const PageMeta *, 16> pages;
	bool found_current = false;
	for (unsigned i = 0; i < 10; ++i) {
		const auto *page = n_pages[i];
		if (page == nullptr) {
			page = f_pages[i];
			if (page == nullptr)
				continue;
		}

		if (page == &current_page_meta)
			found_current = true;

		pages.emplace_back(page);
	}

	if (!found_current)
		pages.emplace_back(&current_page_meta);

	/* now draw the pages we collected */
	tabs.clear();
	unsigned x = 0;
	for (const auto *i : pages) {
		const auto &page = *i;

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
