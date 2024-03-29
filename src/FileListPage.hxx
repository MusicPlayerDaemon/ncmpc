// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef FILE_LIST_PAGE_HXX
#define FILE_LIST_PAGE_HXX

#include "config.h"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"

#include <curses.h>

#include <memory>

struct mpdclient;
struct MpdQueue;
class ScreenManager;
class FileList;
struct FileListEntry;

class FileListPage : public ListPage, ListRenderer, ListText {
protected:
	ScreenManager &screen;

	std::unique_ptr<FileList> filelist;
	const char *const song_format;

public:
	FileListPage(ScreenManager &_screen, WINDOW *_w,
		     Size size,
		     const char *_song_format) noexcept;

	~FileListPage() noexcept override;

protected:
	[[gnu::pure]]
	FileListEntry *GetSelectedEntry() const noexcept;

	[[gnu::pure]]
	const struct mpd_entity *GetSelectedEntity() const noexcept;

	[[gnu::pure]]
	const struct mpd_song *GetSelectedSong() const noexcept;

	[[gnu::pure]]
	FileListEntry *GetIndex(unsigned i) const noexcept;

protected:
	virtual bool HandleEnter(struct mpdclient &c);

private:
	bool HandleSelect(struct mpdclient &c) noexcept;
	bool HandleAdd(struct mpdclient &c) noexcept;
	bool HandleEdit(struct mpdclient &c) noexcept;

	void HandleSelectAll(struct mpdclient &c) noexcept;

	static void PaintRow(WINDOW *w, unsigned i,
			     unsigned y, unsigned width,
			     bool selected, const void *data) noexcept;

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
			       bool selected, const char *name) noexcept;

#endif
