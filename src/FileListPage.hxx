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

#ifndef FILE_LIST_PAGE_HXX
#define FILE_LIST_PAGE_HXX

#include "config.h"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"

#include <curses.h>

struct mpdclient;
struct MpdQueue;
class ScreenManager;
class FileList;
struct FileListEntry;

class FileListPage : public ListPage, ListRenderer, ListText {
protected:
	ScreenManager &screen;

	FileList *filelist = nullptr;
	const char *const song_format;

public:
	FileListPage(ScreenManager &_screen, WINDOW *_w,
		     Size size,
		     const char *_song_format)
		:ListPage(_w, size),
		 screen(_screen),
		 song_format(_song_format) {}

	~FileListPage() noexcept override;

protected:
	gcc_pure
	FileListEntry *GetSelectedEntry() const;

	gcc_pure
	const struct mpd_entity *GetSelectedEntity() const;

	gcc_pure
	const struct mpd_song *GetSelectedSong() const;

	FileListEntry *GetIndex(unsigned i) const;

private:
	bool HandleEnter(struct mpdclient &c);
	bool HandleSelect(struct mpdclient &c);
	bool HandleAdd(struct mpdclient &c);

	void HandleSelectAll(struct mpdclient &c);

	static void PaintRow(WINDOW *w, unsigned i,
			     unsigned y, unsigned width,
			     bool selected, const void *data);

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i,
			   unsigned y, unsigned width,
			   bool selected) const noexcept final;

	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const noexcept override;

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	bool PaintStatusBarOverride(const Window &window) const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p,
		     mmask_t bstate) override;
#endif
};

#ifndef NCMPC_MINI

void
screen_browser_sync_highlights(FileList &fl,
			       const MpdQueue &playlist) noexcept;

#else

static inline void
screen_browser_sync_highlights(FileList &, const MpdQueue &) noexcept
{
}

#endif

void
screen_browser_paint_directory(WINDOW *w, unsigned width,
			       bool selected, const char *name);

#endif
