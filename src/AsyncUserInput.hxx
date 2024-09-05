// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "event/PipeEvent.hxx"

#include <curses.h>

class UserInputHandler;

class AsyncUserInput {
	PipeEvent stdin_event;

	WINDOW &w;

	UserInputHandler &handler;

public:
	AsyncUserInput(EventLoop &event_loop, WINDOW &_w,
		       UserInputHandler &_handler) noexcept;

	// TODO remove this kludge
	void InjectKey(int key) noexcept;

private:
	void OnSocketReady(unsigned flags) noexcept;
};

void
keyboard_unread(int key) noexcept;
