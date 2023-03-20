// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef WAIT_USER_INPUT_HXX
#define WAIT_USER_INPUT_HXX

#include <sys/poll.h>
#include <unistd.h>

class WaitUserInput {
	struct pollfd pfd;

public:
	WaitUserInput() noexcept {
		pfd.fd = STDIN_FILENO;
		pfd.events = POLLIN;
	}

	bool IsReady() noexcept {
		return Poll(0);
	}

	bool Wait() noexcept {
		return Poll(-1);
	}

private:
	bool Poll(int timeout) noexcept {
		return poll(&pfd, 1, timeout) > 0;
	}
};

#endif
