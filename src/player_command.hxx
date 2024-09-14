// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

enum class Command : unsigned;
struct mpdclient;
class Interface;
class DelayedSeek;

bool
handle_player_command(Interface &interface,
		      struct mpdclient &c, DelayedSeek &seek, Command cmd);
