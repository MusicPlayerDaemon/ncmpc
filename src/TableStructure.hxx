// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "TableColumn.hxx"
#include "util/StaticVector.hxx"

struct TableStructure {
	StaticVector<TableColumn, 64> columns;
};
