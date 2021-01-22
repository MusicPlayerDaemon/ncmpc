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

#ifndef ASYNC_USER_INPUT_HXX
#define ASYNC_USER_INPUT_HXX

#include "event/SocketEvent.hxx"

#include <curses.h>

class AsyncUserInput {
	SocketEvent socket_event;

	WINDOW &w;

public:
	AsyncUserInput(EventLoop &event_loop, WINDOW &_w) noexcept;

private:
	void OnSocketReady(unsigned flags) noexcept;
};

void
keyboard_unread(EventLoop &event_loop, int key);

#endif
