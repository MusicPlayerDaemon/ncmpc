// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "History.hxx"

#include <string>

namespace Co { class InvokeTask; }
enum class Command : unsigned;
class Interface;
class ModalDock;
class ListWindow;
class ListRenderer;
class ListText;

class FindSupport {
	Interface &interface;
	ModalDock &modal_dock;

	std::string last;
	History history;

public:
	[[nodiscard]]
	FindSupport(Interface &_interface,
		    ModalDock &_modal_dock) noexcept
		:interface(_interface), modal_dock(_modal_dock) {}

	/**
	 * query user for a string and find it in a list window
	 *
	 * @param lw the list window to search
	 * @param findcmd the search command/mode
	 * @param callback_fn a function returning the text of a given line
	 * @param callback_data a pointer passed to callback_fn
	 * @return a task if the command has been handled, an empty task if not
	 */
	[[nodiscard]]
	Co::InvokeTask Find(ListWindow &lw, const ListText &text, Command cmd) noexcept;

	/* query user for a string and jump to the entry
	 * which begins with this string while the users types */
	[[nodiscard]]
	Co::InvokeTask Jump(ListWindow &lw, const ListText &text, const ListRenderer &renderer) noexcept;

private:
	[[nodiscard]]
	Co::InvokeTask DoFind(ListWindow &lw, const ListText &text, bool reversed) noexcept;
};
