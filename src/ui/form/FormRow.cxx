// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "FormRow.hxx"
#include "ui/RowPaint.hxx"
#include "ui/Window.hxx"
#include "util/LocaleString.hxx"
#include "charset.hxx"
#include "Styles.hxx"

FormRow::FormRow(std::string_view _label_locale) noexcept
	:label_locale(_label_locale),
	 label_width(StringWidthMB(label_locale))
{
}

unsigned
FormRow::PaintLabel(const Window window, unsigned width, bool selected) const noexcept
{
	SelectRowStyle(window, Style::LIST_BOLD, selected);
	ClearRowEnd(window, width, selected);

	window.String(label_locale);
	window.Char(':');

	return label_width + 2;
}
