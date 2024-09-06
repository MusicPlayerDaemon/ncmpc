// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "History.hxx"

#include <string>

class Completion;
struct Window;

/**
 *
 * This function calls curs_set(1), to enable cursor.  It will not
 * restore this settings when exiting.
 *
 * @param the curses window to use
 * @param initial_value initial value or nullptr for a empty line;
 * (char *) -1 = get value from history
 * @param history a pointer to a history list or nullptr
 * @param a #Completion instance or nullptr
 */
std::string
wreadln(Window window,
	const char *initial_value,
	History *history,
	Completion *completion) noexcept;

std::string
wreadln_masked(Window window,
	       const char *initial_value) noexcept;
