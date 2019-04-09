/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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

#include "UserInput.hxx"

#include <curses.h>

class AsyncUserInput : public UserInput {
	WINDOW &w;

public:
	AsyncUserInput(boost::asio::io_service &io_service, WINDOW &_w);

private:
	void AsyncWait() {
		UserInput::AsyncWait(std::bind(&AsyncUserInput::OnReadable, this,
					       std::placeholders::_1));
	}

	void OnReadable(const boost::system::error_code &error);
};

void
keyboard_unread(boost::asio::io_service &io_service, int key);

#endif
