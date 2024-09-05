// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "event/SocketEvent.hxx"

class UserInputHandler;

class LircInput final {
	struct lirc_config *lc = nullptr;

	UserInputHandler &handler;

	SocketEvent event;

public:
	LircInput(EventLoop &event_loop,
		  UserInputHandler &_handler) noexcept;
	~LircInput();

	auto &GetEventLoop() const noexcept {
		return event.GetEventLoop();
	}

private:
	void OnSocketReady(unsigned flags) noexcept;
};
