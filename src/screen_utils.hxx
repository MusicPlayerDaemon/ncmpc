// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef SCREEN_UTILS_H
#define SCREEN_UTILS_H

#include "History.hxx"
#include "Completion.hxx"

/* sound an audible and/or visible bell */
void
screen_bell() noexcept;

/* read a character from the status window */
int
screen_getch(const char *prompt) noexcept;

/**
 * display a prompt, wait for the user to press a key, and compare it with
 * the default keys for "yes" and "no" (and their upper-case pendants).
 *
 * @returns true, if the user pressed the key for "yes"; false, if the user
 *	    pressed the key for "no"; def otherwise
 */
bool
screen_get_yesno(const char *prompt, bool def) noexcept;

std::string
screen_read_password(const char *prompt) noexcept;

std::string
screen_readln(const char *prompt, const char *value,
	      History *history, Completion *completion) noexcept;

void
screen_display_completion_list(Completion::Range range) noexcept;

#endif
