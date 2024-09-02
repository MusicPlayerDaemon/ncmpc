// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Page.hxx"
#include "ui/Window.hxx"
#include "config.h"

class ProxyPage : public Page {
	const Window window;

	Page *current_page = nullptr;

	bool is_open = false;

public:
	explicit ProxyPage(const Window _window) noexcept
		:window(_window) {}

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

	std::string_view GetTitle(std::span<char> buffer) const noexcept override;
};
