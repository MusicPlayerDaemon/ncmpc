// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "screen_find.hxx"
#include "screen_utils.hxx"
#include "screen_status.hxx"
#include "screen.hxx"
#include "ListWindow.hxx"
#include "AsyncUserInput.hxx"
#include "i18n.h"
#include "Command.hxx"
#include "Options.hxx"
#include "util/LocaleString.hxx"

#include <ctype.h>

#define FIND_PROMPT  _("Find")
#define RFIND_PROMPT _("Find backward")
#define JUMP_PROMPT _("Jump")

/* query user for a string and find it in a list window */
bool
screen_find(ScreenManager &screen, ListWindow &lw, Command findcmd,
	    const ListText &text) noexcept
{
	bool found;
	const char *prompt = FIND_PROMPT;

	const bool reversed =
		findcmd == Command::LIST_RFIND ||
		findcmd == Command::LIST_RFIND_NEXT;
	if (reversed)
		prompt = RFIND_PROMPT;

	switch (findcmd) {
	case Command::LIST_FIND:
	case Command::LIST_RFIND:
		screen.findbuf.clear();
		/* fall through */

	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		if (screen.findbuf.empty()) {
			char *value = options.find_show_last_pattern
				? (char *) -1 : nullptr;
			screen.findbuf=screen_readln(screen, prompt,
						     value,
						     &screen.find_history,
						     nullptr);
		}

		if (screen.findbuf.empty())
			return true;

		found = reversed
			? lw.ReverseFind(text,
					 screen.findbuf,
					 options.find_wrap,
					 options.bell_on_wrap)
			: lw.Find(text,
				  screen.findbuf,
				  options.find_wrap,
				  options.bell_on_wrap);
		if (!found) {
			screen_status_printf(_("Unable to find \'%s\'"),
					     screen.findbuf.c_str());
			screen_bell();
		}
		return true;
	default:
		break;
	}
	return false;
}

/* query user for a string and jump to the entry
 * which begins with this string while the users types */
void
screen_jump(ScreenManager &screen, ListWindow &lw,
	    const ListText &text,
	    const ListRenderer &renderer) noexcept
{
	constexpr size_t WRLN_MAX_LINE_SIZE = 1024;
	int key = 65;

	char buffer[WRLN_MAX_LINE_SIZE];

	/* In screen.findbuf is the whole string which is displayed in the status_window
	 * and search_str is the string the user entered (without the prompt) */
	char *search_str = buffer + snprintf(buffer, WRLN_MAX_LINE_SIZE, "%s: ", JUMP_PROMPT);
	char *iter = search_str;

	while(1) {
		key = screen_getch(screen, buffer);
		/* if backspace or delete was pressed, process instead of ending loop */
		if (key == KEY_BACKSPACE || key == KEY_DC) {
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

	screen.findbuf = search_str;

	/* ncmpc should get the command */
	keyboard_unread(screen.GetEventLoop(), key);
}
