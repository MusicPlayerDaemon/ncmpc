// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_SCREEN_FIND_H
#define NCMPC_SCREEN_FIND_H

enum class Command : unsigned;
class ScreenManager;
class ListWindow;
class ListRenderer;
class ListText;

/**
 * query user for a string and find it in a list window
 *
 * @param lw the list window to search
 * @param findcmd the search command/mode
 * @param callback_fn a function returning the text of a given line
 * @param callback_data a pointer passed to callback_fn
 * @return true if the command has been handled, false if not
 */
bool
screen_find(ScreenManager &screen, ListWindow &lw,
	    Command findcmd,
	    const ListText &text) noexcept;

/* query user for a string and jump to the entry
 * which begins with this string while the users types */
void
screen_jump(ScreenManager &screen, ListWindow &lw,
	    const ListText &text, const ListRenderer &renderer) noexcept;

#endif
