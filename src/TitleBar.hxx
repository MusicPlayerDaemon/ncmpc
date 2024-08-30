// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_TITLE_BAR_H
#define NCMPC_TITLE_BAR_H

#include "Window.hxx"

struct mpd_status;
struct PageMeta;

class TitleBar {
	UniqueWindow window;

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
		   const char *title) const noexcept;
};

#endif
