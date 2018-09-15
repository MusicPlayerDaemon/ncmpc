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

#ifndef NCMPC_PAGE_META_HXX
#define NCMPC_PAGE_META_HXX

#include <memory>
#include "config.h"
#include "ncmpc_curses.h"
#include "Size.hxx"

enum class Command : unsigned;
class Page;
class ScreenManager;

struct PageMeta {
	const char *name;

	/**
	 * A title/caption for this page, to be translated using
	 * gettext().
	 */
	const char *title;

	/**
	 * The command which switches to this page.
	 */
	Command command;

	std::unique_ptr<Page> (*init)(ScreenManager &screen, WINDOW *w, Size size);
};

#endif
