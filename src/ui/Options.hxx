// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

struct UiOptions {
	unsigned scroll_offset = 0;

	bool find_wrap = true;
	bool find_show_last_pattern = false;
	bool list_wrap = false;
	bool wide_cursor = true;
	bool hardware_cursor;

#ifdef ENABLE_COLORS
	bool enable_colors = true;
#endif
	bool audible_bell = true;
	bool visible_bell;
	bool bell_on_wrap = true;
#ifdef NCMPC_MINI
	static constexpr bool jump_prefix_only = true;
#else
	bool jump_prefix_only = true;
#endif
};

inline UiOptions ui_options;
