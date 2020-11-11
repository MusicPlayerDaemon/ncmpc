/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#ifndef NCMPC_CALLBACKS_H
#define NCMPC_CALLBACKS_H

#include <exception>

struct mpdclient;

/**
 * A connection to MPD has been established.
 */
void
mpdclient_connected_callback() noexcept;

/**
 * An attempt to connect to MPD has failed.
 * mpdclient_error_callback() has been called already.
 */
void
mpdclient_failed_callback() noexcept;

/**
 * The connection to MPD was lost.  If this was due to an error, then
 * mpdclient_error_callback() has already been called.
 */
void
mpdclient_lost_callback() noexcept;

/**
 * To be implemented by the application: mpdclient.c calls this to
 * display an error message.
 *
 * @param message a human-readable error message in the locale charset
 */
void
mpdclient_error_callback(const char *message) noexcept;

void
mpdclient_error_callback(std::exception_ptr e) noexcept;

bool
mpdclient_auth_callback(struct mpdclient *c) noexcept;

void
mpdclient_idle_callback(unsigned events) noexcept;

#endif
