// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef ASYNC_USER_INPUT_HXX
#define ASYNC_USER_INPUT_HXX

#include "event/PipeEvent.hxx"

#include <curses.h>

class AsyncUserInput {
	PipeEvent socket_event;

	WINDOW &w;

public:
	AsyncUserInput(EventLoop &event_loop, WINDOW &_w) noexcept;

private:
	void OnSocketReady(unsigned flags) noexcept;
};

void
keyboard_unread(EventLoop &event_loop, int key);

#endif
