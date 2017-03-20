/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "keyboard.h"
#include "command.h"
#include "ncmpc.h"
#include "ncmpc_curses.h"
#include "screen.h"
#include "Compiler.h"

#include <glib.h>

#include <unistd.h>

gcc_pure
static command_t
translate_key(int key)
{
	if (key == ERR || key == '\0')
		return CMD_NONE;

#ifdef HAVE_GETMOUSE
	if (key == KEY_MOUSE)
		return CMD_MOUSE_EVENT;
#endif

	return get_key_command(key);
}

static command_t
get_keyboard_command(void)
{
	return translate_key(wgetch(screen.main_window.w));
}

static gboolean
keyboard_event(gcc_unused GIOChannel *source,
	       gcc_unused GIOCondition condition,
	       gcc_unused gpointer data)
{
	begin_input_event();

	command_t cmd = get_keyboard_command();
	if (cmd != CMD_NONE)
		if (!do_input_event(cmd))
			return FALSE;

	end_input_event();
	return TRUE;
}

void
keyboard_init(void)
{
	GIOChannel *channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(channel, G_IO_IN, keyboard_event, NULL);
	g_io_channel_unref(channel);
}

void
keyboard_unread(int key)
{
	command_t cmd = translate_key(key);
	if (cmd != CMD_NONE)
		do_input_event(cmd);
}
