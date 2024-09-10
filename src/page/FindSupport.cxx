// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "FindSupport.hxx"
#include "screen.hxx"
#include "screen_utils.hxx"
#include "screen_status.hxx"
#include "screen.hxx"
#include "AsyncUserInput.hxx"
#include "i18n.h"
#include "Command.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "ui/Bell.hxx"
#include "ui/Keys.hxx"
#include "ui/ListWindow.hxx"
#include "ui/Options.hxx"
#include "co/InvokeTask.hxx"
#include "util/LocaleString.hxx"

#include <ctype.h>

#define FIND_PROMPT  _("Find")
#define RFIND_PROMPT _("Find backward")
#define JUMP_PROMPT _("Jump")

inline Co::InvokeTask
FindSupport::DoFind(ListWindow &lw, const ListText &text, bool reversed) noexcept
{
	if (last.empty()) {
		const char *const prompt = reversed ? RFIND_PROMPT : FIND_PROMPT;

		std::string value;
		if (ui_options.find_show_last_pattern && !history.empty())
			value = history.back();

		last = co_await TextInputDialog{
			screen,
			prompt,
			std::move(value),
			&history,
		};
	}

	if (last.empty())
		co_return;

	bool found = reversed
		? lw.ReverseFind(text, last)
		: lw.Find(text, last);
	if (!found) {
		screen_status_printf(_("Unable to find \'%s\'"),
				     last.c_str());
		Bell();
	}
}

Co::InvokeTask
FindSupport::Find(ListWindow &lw, const ListText &text, Command cmd) noexcept
{
	const bool reversed =
		cmd == Command::LIST_RFIND ||
		cmd == Command::LIST_RFIND_NEXT;

	switch (cmd) {
	case Command::LIST_FIND:
	case Command::LIST_RFIND:
		last.clear();
		/* fall through */

	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		return DoFind(lw, text, reversed);

	default:
		return {};
	}
}

void
FindSupport::Jump(ListWindow &lw,
		  const ListText &text,
		  const ListRenderer &renderer) noexcept
{
	constexpr size_t WRLN_MAX_LINE_SIZE = 1024;
	int key = 65;

	char buffer[WRLN_MAX_LINE_SIZE];

	/* In last is the whole string which is displayed in the status_window
	 * and search_str is the string the user entered (without the prompt) */
	char *search_str = buffer + snprintf(buffer, WRLN_MAX_LINE_SIZE, "%s: ", JUMP_PROMPT);
	char *iter = search_str;

	while(1) {
		key = screen_getch(screen, buffer);
		/* if backspace or delete was pressed, process instead of ending loop */
		if (IsBackspace(key) || key == KEY_DC) {
			const char *prev = PrevCharMB(buffer, iter);
			if (search_str <= prev)
				iter = const_cast<char *>(prev);
			*iter = '\0';
			continue;
		}
		/* if a control key was pressed, end loop */
		else if (iscntrl(key) || key == KEY_NPAGE || key == KEY_PPAGE) {
			break;
		}
		else if (iter < buffer + WRLN_MAX_LINE_SIZE - 3) {
			*iter++ = key;
			*iter = '\0';
		}
		lw.Jump(text, search_str);

		/* repaint the list_window */
		lw.Paint(renderer);
		lw.Refresh();
	}

	last = search_str;

	/* ncmpc should get the command */
	keyboard_unread(key);
}
