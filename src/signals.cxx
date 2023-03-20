// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include <signal.h>
#include "Instance.hxx"

void
Instance::OnSigwinch() noexcept
{
	endwin();
	refresh();
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
