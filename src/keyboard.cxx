/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "config.h"
#include "keyboard.hxx"
#include "Command.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "ncmpc.hxx"
#include "Point.hxx"
#include "util/Compiler.h"

static bool
ignore_key(int key)
{
	return key == ERR || key == '\0';
}

gcc_pure
static Command
translate_key(int key)
{
	return GetGlobalKeyBindings().FindKey(key);
}

void
UserInput::OnReadable(const boost::system::error_code &error)
{
	if (error) {
		get_io_context().stop();
		return;
	}

	int key = wgetch(&w);
	if (ignore_key(key)) {
		AsyncWait();
		return;
	}

#ifdef HAVE_GETMOUSE
	if (key == KEY_MOUSE) {
		MEVENT event;

		/* retrieve the mouse event from curses */
#ifdef PDCURSES
		nc_getmouse(&event);
#else
		getmouse(&event);
#endif

		begin_input_event();
		do_mouse_event({event.x, event.y}, event.bstate);
		end_input_event();

		AsyncWait();
		return;
	}
#endif

	Command cmd = translate_key(key);
	if (cmd == Command::NONE) {
		AsyncWait();
		return;
	}

	begin_input_event();

	if (!do_input_event(get_io_context(), cmd))
		return;

	end_input_event();
	AsyncWait();
}

UserInput::UserInput(boost::asio::io_service &io_service, WINDOW &_w)
	:d(io_service),
#if BOOST_VERSION >= 107000
	 io_context(io_service),
#endif
	 w(_w)
{
	d.assign(STDIN_FILENO);
	AsyncWait();
}

void
keyboard_unread(boost::asio::io_service &io_service, int key)
{
	if (ignore_key(key))
		return;

	Command cmd = translate_key(key);
	if (cmd != Command::NONE)
		do_input_event(io_service, cmd);
}
