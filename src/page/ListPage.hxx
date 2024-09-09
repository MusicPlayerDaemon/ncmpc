// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Page.hxx"
#include "ui/ListWindow.hxx"

/**
 * An abstract #Page implementation which shows a #ListWindow.
 */
class ListPage : public Page {
protected:
	ListWindow lw;

	ListPage(PageContainer &_parent, Window window, Size size) noexcept
		:Page(_parent), lw(window, size) {}

public:
	/* virtual methods from class Page */
	void OnResize(Size size) noexcept override {
		lw.Resize(size);
	}

	bool OnCommand(struct mpdclient &, Command cmd) override {
		if (lw.HasCursor()
		    ? lw.HandleCommand(cmd)
		    : lw.HandleScrollCommand(cmd)) {
			SchedulePaint();
			return true;
		}

		return false;
	}

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &, Point p,
		     mmask_t bstate) override {
		if (lw.HandleMouse(bstate, p.y)) {
			SchedulePaint();
			return true;
		}

		return false;
	}

#endif
};
