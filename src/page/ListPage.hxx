// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Page.hxx"
#include "ui/ListWindow.hxx"

namespace Co { class InvokeTask; }
class ModalDock;

/**
 * An abstract #Page implementation which shows a #ListWindow.
 */
class ListPage : public Page {
protected:
	ListWindow lw;

	ListPage(PageContainer &_parent, Window window) noexcept
		:Page(_parent), lw(window) {}

public:
	unsigned GetWidth() const noexcept {
		return lw.GetWidth();
	}

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

protected:
	/**
	 * Query user for a string and jump to the entry which begins
	 * with this string while the users types.
	 */
	[[nodiscard]]
	Co::InvokeTask Jump(ModalDock &modal_dock,
			    const ListText &text) noexcept;
};
