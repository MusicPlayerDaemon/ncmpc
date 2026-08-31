// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "StringEditRow.hxx"
#include "ui/dialogs/StringEditDialog.hxx"
#include "ui/paint.hxx"
#include "ui/Window.hxx"
#include "Styles.hxx"

void
StringEditRow::Paint(const Window window, unsigned y, unsigned width,
		     bool selected) const noexcept
{
	unsigned x = PaintLabel(window, width, selected);

	row_color(window, Style::LIST, selected);
	window.String({x, y}, value);
}

Co::Task<bool>
StringEditRow::Edit(ModalDock &modal_dock) noexcept
{
	return StringEditDialog(modal_dock, GetLabel(), value);
}
