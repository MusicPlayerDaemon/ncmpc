// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string_view>

struct Window;
struct PageMeta;

void
PaintTabBar(Window window, const PageMeta &current_page_meta,
	    std::string_view current_page_title) noexcept;

[[gnu::pure]]
const PageMeta *
GetTabAtX(const PageMeta &current_page_meta,
	  std::string_view current_page_title,
	  unsigned x) noexcept;
