// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Page.hxx"
#include "Container.hxx"
#include "ui/Window.hxx"
#include "config.h"

class ProxyPage : public Page, public PageContainer {
	const Window window;

	Page *current_page = nullptr;

	bool is_open = false;

public:
	explicit ProxyPage(PageContainer &_parent, const Window _window) noexcept
		:Page(_parent), window(_window) {}

	[[nodiscard]]
	const Page *GetCurrentPage() const noexcept {
		return current_page;
	}

	[[nodiscard]]
	Page *GetCurrentPage() noexcept {
		return current_page;
	}

	void SetCurrentPage(struct mpdclient &c, Page *new_page) noexcept;

	using Page::SchedulePaint;

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
	const struct mpd_song *GetSelectedSong() const noexcept override;

	// virtual methods from PageContainer
	void SchedulePaint(Page &page) noexcept override;
	void Alert(std::string message) noexcept override;
};
