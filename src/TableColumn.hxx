// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string>

struct TableColumn {
	std::string caption, format;

	unsigned min_width = 10;

	float fraction_width = 1;
};
