// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string_view>

struct Window;

class FormRow {
	const std::string_view label_locale;

	unsigned label_width;

public:
	[[nodiscard]]
	explicit FormRow(std::string_view _label_locale) noexcept;

	[[gnu::pure]]
	unsigned GetLabelWidth() const noexcept {
		return label_width;
	}

	void SetLabelWidth(unsigned _width) noexcept {
		label_width = _width;
	}

	unsigned PaintLabel(const Window window, unsigned width, bool selected) const noexcept;

protected:
	std::string_view GetLabel() const noexcept {
		return label_locale;
	}
};
