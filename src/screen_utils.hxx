// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef SCREEN_UTILS_H
#define SCREEN_UTILS_H

#include "Completion.hxx"

class ScreenManager;

void
screen_display_completion_list(ScreenManager &screen,
			       Completion::Range range) noexcept;

#endif
