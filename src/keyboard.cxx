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
#include "command.hxx"
#include "ncmpc.hxx"
#include "ncmpc_curses.h"
#include "screen.hxx"
#include "Compiler.h"

#include <glib.h>

#include <unistd.h>

static bool
ignore_key(int key)
{
	return key == ERR || key == '\0';
}

gcc_pure
static command_t
translate_key(int key)
{
	return get_key_command(key);
}

static gboolean
keyboard_event(gcc_unused GIOChannel *source,
	       gcc_unused GIOCondition condition,
	       gpointer data)
{
	auto *w = (WINDOW *)data;

	int key = wgetch(w);
	if (ignore_key(key))
		return true;

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
		do_mouse_event(event.x, event.y, event.bstate);
		end_input_event();

		return true;
	}
#endif

	command_t cmd = translate_key(key);
	if (cmd == CMD_NONE)
		return true;

	begin_input_event();

	if (!do_input_event(cmd))
		return false;

	end_input_event();
	return true;
}

void
keyboard_init(WINDOW *w)
{
	GIOChannel *channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(channel, G_IO_IN, keyboard_event, w);
	g_io_channel_unref(channel);
}

void
keyboard_unread(int key)
{
	if (ignore_key(key))
		return;

	command_t cmd = translate_key(key);
	if (cmd != CMD_NONE)
		do_input_event(cmd);
}
