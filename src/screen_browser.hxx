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

#ifndef SCREEN_BROWSER_H
#define SCREEN_BROWSER_H

#include "command.hxx"
#include "config.h"
#include "ncmpc_curses.h"
#include "ListPage.hxx"

struct mpdclient;
struct MpdQueue;
class ScreenManager;
class FileList;
struct FileListEntry;

class FileListPage : public ListPage {
protected:
	ScreenManager &screen;

	FileList *filelist = nullptr;
	const char *const song_format;

public:
	FileListPage(ScreenManager &_screen, WINDOW *_w,
		     unsigned _cols, unsigned _rows,
		     const char *_song_format)
		:ListPage(_w, _cols, _rows),
		 screen(_screen),
		 song_format(_song_format) {}

	~FileListPage() override;

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

public:
	/* virtual methods from class Page */
	void Paint() const override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p,
		     mmask_t bstate) override;
#endif
};

#ifndef NCMPC_MINI

void
screen_browser_sync_highlights(FileList *fl,
			       const MpdQueue *playlist);

#else

static inline void
screen_browser_sync_highlights(gcc_unused FileList *fl,
			       gcc_unused const MpdQueue *playlist)
{
}

#endif

void
screen_browser_paint_directory(WINDOW *w, unsigned width,
			       bool selected, const char *name);

#endif
