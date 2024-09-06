// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string>

class ScreenManager;
struct mpdclient;
namespace Co { class InvokeTask; }

[[nodiscard]]
Co::InvokeTask
playlist_save(ScreenManager &screen, struct mpdclient &c,
	      std::string name={}) noexcept;
