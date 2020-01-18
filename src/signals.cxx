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

#include "Instance.hxx"

void
Instance::OnSigwinch()
{
	endwin();
	refresh();
	screen_manager.OnResize();
}

void
Instance::AsyncWaitSigwinch()
{
	sigwinch.async_wait([this](const auto &error, int){
			if (error)
				return;

			this->OnSigwinch();
			this->AsyncWaitSigwinch();
		});
}

void
Instance::InitSignals()
{
	AsyncWaitSigwinch();

	/* ignore SIGPIPE */

	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, nullptr) < 0) {
		perror("sigaction(SIGPIPE)");
		exit(EXIT_FAILURE);
	}
}
