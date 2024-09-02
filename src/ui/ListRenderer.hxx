// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

struct Window;

class ListRenderer {
public:
	virtual void PaintListItem(Window window, unsigned i,
				   unsigned y, unsigned width,
				   bool selected) const noexcept = 0;
};
