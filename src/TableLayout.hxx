// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <array>

struct TableStructure;

struct TableColumnLayout {
	unsigned width;
};

struct TableLayout {
	const TableStructure &structure;

	std::array<TableColumnLayout, 64> columns;

	explicit TableLayout(const TableStructure &_structure) noexcept
		:structure(_structure) {}

	void Calculate(unsigned screen_width) noexcept;
};
