// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "TableColumn.hxx"

#include <vector>

struct TableStructure {
	std::vector<TableColumn> columns;
};
