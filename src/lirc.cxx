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

#include "lirc.hxx"
#include "ncmpc.hxx"
#include "Compiler.h"

#include <lirc/lirc_client.h>

#include <glib.h>

static struct lirc_config *lc = nullptr;

static gboolean
lirc_event(gcc_unused GIOChannel *source,
	   gcc_unused GIOCondition condition, gcc_unused gpointer data)
{
	char *code, *txt;
	command_t cmd;

	begin_input_event();

	if (lirc_nextcode(&code) == 0) {
		while (lirc_code2char(lc, code, &txt) == 0 && txt != nullptr) {
			cmd = get_key_command_from_name(txt);
			if (!do_input_event(cmd))
				return false;
		}
	}

	end_input_event();
	return true;
}

void
ncmpc_lirc_init()
{
	char prog[] = "ncmpc";
	int lirc_socket = 0;

	if ((lirc_socket = lirc_init(prog, 0)) == -1)
		return;

	if (lirc_readconfig(nullptr, &lc, nullptr)) {
		lirc_deinit();
		return;
	}

	GIOChannel *channel = g_io_channel_unix_new(lirc_socket);
	g_io_add_watch(channel, G_IO_IN, lirc_event, nullptr);
	g_io_channel_unref(channel);
}

void
ncmpc_lirc_deinit()
{
	if (lc)
		lirc_freeconfig(lc);
	lirc_deinit();
}
