// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include <signal.h>
#include "Instance.hxx"

void
Instance::OnSigwinch() noexcept
{
	endwin();
	refresh();

	/* re-enable keypad mode; it got disabled by endwin() and
	   ncurses will only automatically re-enable it when calling
	   wgetch(), but we will do that only after STDIN has become
	   readable, when a keystroke without keypad mode has already
	   been put into STDIN by the terminal */
	keypad(stdscr, true);

	screen_manager.OnResize();
}

void
Instance::InitSignals()
{
	/* ignore SIGPIPE */

	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, nullptr) < 0) {
		perror("sigaction(SIGPIPE)");
		exit(EXIT_FAILURE);
	}
}
