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

#ifndef NCMPC_ARTIST_LIST_PAGE_HXX
#define NCMPC_ARTIST_LIST_PAGE_HXX

#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"

#include <vector>
#include <string>

class ScreenManager;

class ArtistListPage final : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;
	std::vector<std::string> artist_list;

public:
	ArtistListPage(ScreenManager &_screen, WINDOW *_w, Size _size)
		:ListPage(_w, _size), screen(_screen) {}

	const char *GetSelectedValue() const {
		return lw.selected < artist_list.size()
			? artist_list[lw.selected].c_str()
			: nullptr;
	}

private:
	void LoadArtistList(struct mpdclient &c);
	void Reload(struct mpdclient &c);

	bool OnListCommand(command_t cmd);

public:
	/* virtual methods from class Page */
	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i, unsigned y, unsigned width,
			   bool selected) const override;

	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const override;
};

#endif
