// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_PLAYER_COMMAND_H
#define NCMPC_PLAYER_COMMAND_H

enum class Command : unsigned;
struct mpdclient;
class DelayedSeek;

bool
handle_player_command(struct mpdclient &c, DelayedSeek &seek, Command cmd);

#endif
