// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_PROXY_PAGE_HXX
#define NCMPC_PROXY_PAGE_HXX

#include "Page.hxx"
#include "config.h"

class ProxyPage : public Page {
	WINDOW *const w;

	Page *current_page = nullptr;

	bool is_open = false;

public:
	explicit ProxyPage(WINDOW *_w) noexcept:w(_w) {}

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
	void OnOpen(struct mpdclient &c) noexcept override;
	void OnClose() noexcept override;
	void OnResize(Size size) noexcept override;
	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	const char *GetTitle(char *s, size_t size) const noexcept override;
};

#endif
