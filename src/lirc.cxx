/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#include "lirc.hxx"
#include "ncmpc.hxx"
#include "Command.hxx"
#include "config.h"

#include <lirc/lirc_client.h>

void
LircInput::OnSocketReady(unsigned) noexcept
{
	char *code, *txt;

	begin_input_event();

	if (lirc_nextcode(&code) == 0) {
		while (lirc_code2char(lc, code, &txt) == 0 && txt != nullptr) {
			const auto cmd = get_key_command_from_name(txt);
			if (!do_input_event(GetEventLoop(), cmd))
				return;
		}
	}

	end_input_event();
}

LircInput::LircInput(EventLoop &_event_loop) noexcept
	:event(_event_loop, BIND_THIS_METHOD(OnSocketReady))
{
	int lirc_socket = 0;

	if ((lirc_socket = lirc_init(PACKAGE, 0)) == -1)
		return;

	if (lirc_readconfig(nullptr, &lc, nullptr)) {
		lirc_deinit();
		return;
	}

	event.Open(SocketDescriptor(lirc_socket));
	event.ScheduleRead();
}

LircInput::~LircInput()
{
	if (lc)
		lirc_freeconfig(lc);
	if (event.IsDefined())
		lirc_deinit();
}
