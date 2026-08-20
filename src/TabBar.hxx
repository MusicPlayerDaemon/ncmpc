// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string_view>

struct Window;
struct PageMeta;

class TabBar {
public:
	void Paint(Window window, const PageMeta &current_page_meta,
		   std::string_view current_page_title) const noexcept;

	[[gnu::pure]]
	const PageMeta *GetTabAtX(const PageMeta &current_page_meta,
				  std::string_view current_page_title,
				  unsigned x) const noexcept;
};
