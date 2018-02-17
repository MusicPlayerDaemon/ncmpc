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

#ifndef NCMPC_PLAYER_COMMAND_H
#define NCMPC_PLAYER_COMMAND_H

#include "command.hxx"

struct mpdclient;

extern int seek_id;
extern int seek_target_time;

/**
 * Call this before exiting; it will unschedule the timer for delayed
 * seeking.
 */
void
cancel_seek_timer();

bool
handle_player_command(struct mpdclient *c, command_t cmd);

#endif
