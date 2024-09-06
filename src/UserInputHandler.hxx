// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

#include <curses.h>

enum class Command : unsigned;
struct Point;

/**
 * Handler for input events from the user, called by #AsyncUserInput.
 */
class UserInputHandler {
public:
	/**
	 * A hot key for a #Command was pressed.
	 *
	 * @return false if ncmpc will quit
	 */
	virtual bool OnCommand(Command cmd) noexcept = 0;

#ifdef HAVE_GETMOUSE
	/**
	 * An input event from the mouse was detected.
	 *
	 * @param p the position of the mouse cursor on the screen
	 * @param bstate the state of all mouse buttons
	 *
	 * @return false if ncmpc will quit
	 */
	virtual bool OnMouse(Point p, mmask_t bstate) noexcept = 0;
#endif
};
