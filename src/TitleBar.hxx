/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#ifndef NCMPC_TITLE_BAR_H
#define NCMPC_TITLE_BAR_H

#include "Window.hxx"

struct mpd_status;
struct PageMeta;

class TitleBar {
	Window window;

	int volume;
	char flags[8];

public:
	TitleBar(Point p, unsigned width) noexcept;

	static constexpr unsigned GetHeight() noexcept {
		return 2;
	}

	void OnResize(unsigned width) noexcept;
	void Update(const struct mpd_status *status) noexcept;
	void Paint(const PageMeta &current_page_meta,
		   const char *title) const noexcept;
};

#endif
