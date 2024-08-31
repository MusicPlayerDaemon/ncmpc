// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TextListRenderer.hxx"
#include "ListText.hxx"
#include "paint.hxx"

#include <assert.h>

static void
list_window_paint_row(const Window window, unsigned width, bool selected,
		      std::string_view text) noexcept
{
	row_paint_text(window, width, Style::LIST,
		       selected, text);
}

void
TextListRenderer::PaintListItem(const Window window, unsigned i, unsigned,
				unsigned width, bool selected) const noexcept
{
	char buffer[1024];
	const std::string_view label = text.GetListItemText(buffer, i);

	list_window_paint_row(window, width, selected, label);
}
