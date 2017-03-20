// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef SCREEN_UTILS_H
#define SCREEN_UTILS_H

#include "History.hxx"
#include "Completion.hxx"

class ScreenManager;

/* read a character from the status window */
int
screen_getch(ScreenManager &screen, const char *prompt) noexcept;

std::string
screen_read_password(ScreenManager &screen, const char *prompt) noexcept;

std::string
screen_readln(ScreenManager &screen, const char *prompt,
	      const char *value,
	      History *history, Completion *completion) noexcept;

void
screen_display_completion_list(ScreenManager &screen,
			       Completion::Range range) noexcept;

#endif
