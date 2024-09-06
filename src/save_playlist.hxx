// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

class ScreenManager;
struct mpdclient;

int
playlist_save(ScreenManager &screen, struct mpdclient &c,
	      const char *name) noexcept;
