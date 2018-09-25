/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#ifndef NCMPC_ALBUM_LIST_PAGE_HXX
#define NCMPC_ALBUM_LIST_PAGE_HXX

#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"

#include <vector>
#include <string>

class ScreenManager;

class AlbumListPage final : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;
	Page *const parent;
	std::vector<std::string> album_list;
	std::string artist;

public:
	AlbumListPage(ScreenManager &_screen, Page *_parent,
		      WINDOW *_w, Size size) noexcept
		:ListPage(_w, size), screen(_screen), parent(_parent) {}

	template<typename A>
	void SetArtist(A &&_artist) {
		artist = std::forward<A>(_artist);
		AddPendingEvents(~0u);
	}

	const std::string &GetArtist() const {
		return artist;
	}

	bool IsShowAll() const {
		return lw.selected == album_list.size() + 1;
	}

	const char *GetSelectedValue() const {
		return lw.selected >= 1 && lw.selected <= album_list.size()
			? album_list[lw.selected - 1].c_str()
			: nullptr;
	}

private:
	void LoadAlbumList(struct mpdclient &c);
	void Reload(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i, unsigned y, unsigned width,
			   bool selected) const override;

	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const override;
};

#endif
