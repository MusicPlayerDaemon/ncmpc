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
	 * Implementing this method gives the handler a chance to
	 * handle a key code pressed by the user.  This is called
	 * before looking up hot keys.
	 *
	 * @return true if the character was consumed, false to pass
	 * it on to the hot key checker
	 */
	virtual bool OnRawKey(int key) noexcept = 0;

	/**
	 * A hot key for a #Command was pressed.
	 */
	virtual void OnCommand(Command cmd) noexcept = 0;

#ifdef HAVE_GETMOUSE
	/**
	 * An input event from the mouse was detected.
	 *
	 * @param p the position of the mouse cursor on the screen
	 * @param bstate the state of all mouse buttons
	 */
	virtual void OnMouse(Point p, mmask_t bstate) noexcept = 0;
#endif

	/**
	 * Cancel the topmost modal dialog.
	 *
	 * @return true if a modal dialog was canceled, false if there
	 * was none
	 */
	virtual bool CancelModalDialog() noexcept = 0;
};
