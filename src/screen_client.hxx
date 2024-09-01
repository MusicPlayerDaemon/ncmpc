// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

struct mpdclient;

bool
screen_auth(struct mpdclient &c);

/**
 * Starts a (server-side) database update and displays a status
 * message.
 */
void
screen_database_update(struct mpdclient &c, const char *path);
