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

#include "Instance.hxx"
#include "Options.hxx"
#include "keyboard.hxx"
#include "lirc.hxx"
#include "signals.hxx"
#include "xterm_title.hxx"

#include <glib.h>

Instance::Instance()
	:main_loop(g_main_loop_new(nullptr, false)),
	 client(options.host.empty() ? nullptr : options.host.c_str(),
		options.port,
		options.timeout_ms,
		options.password.empty() ? nullptr : options.password.c_str())

{
	screen_manager.Init(&client);

	/* watch out for keyboard input */
	keyboard_init(screen_manager.main_window.w);

	/* watch out for lirc input */
	ncmpc_lirc_init();

	signals_init(main_loop, screen_manager);
}

Instance::~Instance()
{
	g_main_loop_unref(main_loop);

	signals_deinit();
	ncmpc_lirc_deinit();

	screen_manager.Exit();
}

void
Instance::Run()
{
	screen_manager.Update(client);

	g_main_loop_run(main_loop);
}

