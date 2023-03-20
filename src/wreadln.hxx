// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef WREADLN_H
#define WREADLN_H

#include "History.hxx"

#include <curses.h>

#include <string>

class Completion;

/**
 *
 * This function calls curs_set(1), to enable cursor.  It will not
 * restore this settings when exiting.
 *
 * @param the curses window to use
 * @param initial_value initial value or nullptr for a empty line;
 * (char *) -1 = get value from history
 * @param x1 the maximum x position or 0
 * @param history a pointer to a history list or nullptr
 * @param a #Completion instance or nullptr
 */
std::string
wreadln(WINDOW *w,
	const char *initial_value,
	unsigned x1,
	History *history,
	Completion *completion) noexcept;

std::string
wreadln_masked(WINDOW *w,
	       const char *initial_value,
	       unsigned x1) noexcept;

#endif
