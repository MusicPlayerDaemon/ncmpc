// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "FormRow.hxx"

#include <algorithm> // for std::max()
#include <concepts> // for std::derived_from

template<std::derived_from<FormRow>... Rows>
[[gnu::pure]]
unsigned
MaxLabelWidth(Rows... rows) noexcept
{
	unsigned max_width = 0;
	((max_width = std::max(max_width, rows.GetLabelWidth())), ...);
	return max_width;
}

template<class... R>
void
AdjustLabelWidths(R... rows) noexcept
{
	const unsigned label_width = MaxLabelWidth(rows...);
	(rows.SetLabelWidth(label_width), ...);
}
