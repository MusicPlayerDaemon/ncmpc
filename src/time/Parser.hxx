// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string_view>

#include <time.h>

/**
 * Throws on error.
 */
time_t
ParseDuration(std::string_view s);
