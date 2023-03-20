// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
