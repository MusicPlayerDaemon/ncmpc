// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "ui/Window.hxx"

#ifndef NCMPC_MINI
#include "TabBar.hxx"
#endif

#include <string_view>

struct mpd_status;
struct PageMeta;

class TitleBar {
	UniqueWindow window;

#ifndef NCMPC_MINI
	TabBar tab_bar;
#endif

	int volume;
	char flags[8];

public:
	TitleBar(Point p, unsigned width) noexcept;

	static constexpr unsigned GetHeight() noexcept {
		return 2;
	}

	void OnResize(unsigned width) noexcept;
	void Update(const struct mpd_status *status) noexcept;
	void Paint(const PageMeta &current_page_meta,
		   std::string_view title) const noexcept;

#ifndef NCMPC_MINI
	[[gnu::pure]]
	const PageMeta *GetTabAtX(unsigned x) const noexcept {
		return tab_bar.GetTabAtX(x);
	}
#endif // NCMPC_MINI
};
