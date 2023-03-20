// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_SCREEN_CLIENT_H
#define NCMPC_SCREEN_CLIENT_H


struct mpdclient;

bool
screen_auth(struct mpdclient *c);

/**
 * Starts a (server-side) database update and displays a status
 * message.
 */
void
screen_database_update(struct mpdclient *c, const char *path);

#endif
