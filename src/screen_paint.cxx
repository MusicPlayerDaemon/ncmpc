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

#include "screen.hxx"
#include "Page.hxx"
#include "config.h"
#include "mpdclient.hxx"
#include "Options.hxx"

#include <mpd/client.h>

#include <assert.h>

void
ScreenManager::PaintTopWindow()
{
	const char *title = current_page->second->GetTitle(buf, buf_size);
	assert(title != nullptr);

	title_bar.Paint(GetCurrentPageMeta(), title);
}

void
ScreenManager::Paint(bool main_dirty)
{
	/* update title/header window */
	PaintTopWindow();

	/* paint the bottom window */

	progress_bar.Paint();
	status_bar.Paint();

	/* paint the main window */

	if (main_dirty) {
		current_page->second->Paint();
		current_page->second->SetDirty(false);
	}

	/* move the cursor to the origin */

	if (!options.hardware_cursor)
		wmove(main_window.w, 0, 0);

	wnoutrefresh(main_window.w);

	/* tell curses to update */
	doupdate();
}
