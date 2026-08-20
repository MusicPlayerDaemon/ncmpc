// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "util/StaticVector.hxx"

#include <string_view>

struct Window;
struct PageMeta;

class TabBar {
	struct Tab {
		const PageMeta &meta;
		unsigned width;
	};

	mutable StaticVector<Tab, 16> tabs;

public:
	void Paint(Window window, const PageMeta &current_page_meta,
		   std::string_view current_page_title) const noexcept;

	[[gnu::pure]]
	const PageMeta *GetTabAtX(unsigned x) const noexcept;
};
