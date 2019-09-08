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

#include "lirc.hxx"
#include "ncmpc.hxx"
#include "Bindings.hxx"
#include "util/Compiler.h"

#include <lirc/lirc_client.h>

void
LircInput::OnReadable(const boost::system::error_code &error)
{
	if (error)
		return;

	char *code, *txt;

	begin_input_event();

	if (lirc_nextcode(&code) == 0) {
		while (lirc_code2char(lc, code, &txt) == 0 && txt != nullptr) {
			const auto cmd = get_key_command_from_name(txt);
			if (!do_input_event(get_io_context(), cmd))
				return;
		}
	}

	end_input_event();
	AsyncWait();
}

LircInput::LircInput(boost::asio::io_service &io_service)
	:d(io_service)
#if BOOST_VERSION >= 107000
	, io_context(io_service)
#endif
{
	int lirc_socket = 0;

	if ((lirc_socket = lirc_init(PACKAGE, 0)) == -1)
		return;

	if (lirc_readconfig(nullptr, &lc, nullptr)) {
		lirc_deinit();
		return;
	}

	d.assign(lirc_socket);
	AsyncWait();
}

LircInput::~LircInput()
{
	if (lc)
		lirc_freeconfig(lc);
	if (d.is_open())
		lirc_deinit();
}
