// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "ListRenderer.hxx"

class ListText;

class TextListRenderer final : public ListRenderer {
	const ListText &text;

public:
	explicit TextListRenderer(const ListText &_text) noexcept
		:text(_text) {}

	/* virtual methods from class ListRenderer */
	void PaintListItem(const Window window, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;
};
