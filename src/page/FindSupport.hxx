// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "History.hxx"

#include <string>

enum class Command : unsigned;
class ScreenManager;
class ListWindow;
class ListRenderer;
class ListText;

class FindSupport {
	ScreenManager &screen;

	std::string last;
	History history;

public:
	explicit FindSupport(ScreenManager &_screen) noexcept
		:screen(_screen) {}

	/**
	 * query user for a string and find it in a list window
	 *
	 * @param lw the list window to search
	 * @param findcmd the search command/mode
	 * @param callback_fn a function returning the text of a given line
	 * @param callback_data a pointer passed to callback_fn
	 * @return true if the command has been handled, false if not
	 */
	bool Find(ListWindow &lw, const ListText &text, Command cmd) noexcept;

	/* query user for a string and jump to the entry
	 * which begins with this string while the users types */
	void Jump(ListWindow &lw, const ListText &text, const ListRenderer &renderer) noexcept;
};
