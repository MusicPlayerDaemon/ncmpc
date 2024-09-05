// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

#include <curses.h>

enum class Command : unsigned;
struct Point;

class UserInputHandler {
public:
	virtual bool OnCommand(Command cmd) noexcept = 0;

#ifdef HAVE_GETMOUSE
	virtual bool OnMouse(Point p, mmask_t bstate) noexcept = 0;
#endif
};
