// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_LIST_PAGE_HXX
#define NCMPC_LIST_PAGE_HXX

#include "Page.hxx"
#include "ListWindow.hxx"

/**
 * An abstract #Page implementation which shows a #ListWindow.
 */
class ListPage : public Page {
protected:
	ListWindow lw;

public:
	ListPage(WINDOW *w, Size size)
		:lw(w, size) {}

public:
	/* virtual methods from class Page */
	void OnResize(Size size) noexcept override {
		lw.Resize(size);
	}

	bool OnCommand(struct mpdclient &, Command cmd) override {
		if (lw.HasCursor()
		    ? lw.HandleCommand(cmd)
		    : lw.HandleScrollCommand(cmd)) {
			SetDirty();
			return true;
		}

		return false;
	}

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &, Point p,
		     mmask_t bstate) override {
		if (lw.HandleMouse(bstate, p.y)) {
			SetDirty();
			return true;
		}

		return false;
	}

#endif
};

#endif
