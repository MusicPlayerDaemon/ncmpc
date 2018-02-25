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

#ifndef NCMPC_PROXY_PAGE_HXX
#define NCMPC_PROXY_PAGE_HXX

#include "Page.hxx"

class ProxyPage : public Page {
	WINDOW *const w;

	Page *current_page = nullptr;

	bool is_open = false;

public:
	explicit ProxyPage(WINDOW *_w):w(_w) {}

	const Page *GetCurrentPage() const {
		return current_page;
	}

	Page *GetCurrentPage() {
		return current_page;
	}

	void SetCurrentPage(struct mpdclient &c, Page *new_page);

private:
	void MoveDirty() {
		if (current_page != nullptr && current_page->IsDirty()) {
			current_page->SetDirty(false);
			SetDirty();
		}
	}

public:
	/* virtual methods from Page */
	void OnOpen(struct mpdclient &c) override;
	void OnClose() override;
	void OnResize(Size size) override;
	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	const char *GetTitle(char *s, size_t size) const override;
};

#endif
