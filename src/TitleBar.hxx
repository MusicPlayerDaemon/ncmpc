// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Window.hxx"

#include <string_view>

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
		   std::string_view title) const noexcept;
};
