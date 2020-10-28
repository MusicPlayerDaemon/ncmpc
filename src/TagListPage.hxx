/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef NCMPC_TAG_LIST_PAGE_HXX
#define NCMPC_TAG_LIST_PAGE_HXX

#include "TagFilter.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"

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
	TagListPage(ScreenManager &_screen, Page *_parent,
		    const enum mpd_tag_type _tag,
		    const char *_all_text,
		    WINDOW *_w, Size size) noexcept
		:ListPage(_w, size), screen(_screen), parent(_parent),
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

	gcc_pure
	bool HasMultipleValues() const noexcept {
		return values.size() > 1;
	}

	gcc_pure
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

	const char *GetTitle(char *s, size_t size) const noexcept override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;

	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const noexcept override;
};

#endif
