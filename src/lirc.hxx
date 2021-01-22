/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
