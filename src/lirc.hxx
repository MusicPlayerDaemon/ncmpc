// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef LIRC_H
#define LIRC_H

#include "event/SocketEvent.hxx"

class LircInput final {
	struct lirc_config *lc = nullptr;

	SocketEvent event;

public:
	explicit LircInput(EventLoop &event_loop) noexcept;
	~LircInput();

	auto &GetEventLoop() const noexcept {
		return event.GetEventLoop();
	}

private:
	void OnSocketReady(unsigned flags) noexcept;
};

#endif
