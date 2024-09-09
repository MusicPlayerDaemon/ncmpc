// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "TagFilter.hxx"
#include "page/ListPage.hxx"
#include "ui/ListRenderer.hxx"
#include "ui/ListText.hxx"

#include <vector>
#include <string>

class ScreenManager;

class TagListPage : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;
	Page *const parent;

	const enum mpd_tag_type tag;
	const char *const all_text;

	TagFilter filter;
	std::string title;

	std::vector<std::string> values;

public:
	TagListPage(PageContainer &_container, ScreenManager &_screen, Page *_parent,
		    const enum mpd_tag_type _tag,
		    const char *_all_text,
		    Window _window, Size size) noexcept
		:ListPage(_container, _window, size), screen(_screen), parent(_parent),
		 tag(_tag), all_text(_all_text) {}

	auto GetTag() const noexcept {
		return tag;
	}

	const auto &GetFilter() const noexcept {
		return filter;
	}

	template<typename F>
	void SetFilter(F &&_filter) noexcept {
		filter = std::forward<F>(_filter);
		AddPendingEvents(~0u);
	}

	template<typename T>
	void SetTitle(T &&_title) noexcept {
		title = std::forward<T>(_title);
	}

	/**
	 * Create a filter for the item below the cursor.
	 */
	TagFilter MakeCursorFilter() const noexcept;

	[[gnu::pure]]
	bool HasMultipleValues() const noexcept {
		return values.size() > 1;
	}

	[[gnu::pure]]
	const char *GetSelectedValue() const {
		unsigned i = lw.GetCursorIndex();

		if (parent != nullptr) {
			if (i == 0)
				return nullptr;

			--i;
		}

		return i < values.size()
			? values[i].c_str()
			: nullptr;
	}

protected:
	virtual bool HandleEnter(struct mpdclient &c);

private:
	bool HandleSelect(struct mpdclient &c);

	void LoadValues(struct mpdclient &c) noexcept;
	void Reload(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p,
		     mmask_t bstate) override;
#endif

	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(Window window, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;

	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;
};
