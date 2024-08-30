// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

struct Window;
struct PageMeta;

void
PaintTabBar(Window window, const PageMeta &current_page_meta,
	    const char *current_page_title) noexcept;
