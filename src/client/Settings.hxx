// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string>

struct mpd_settings;

namespace MPD {

#ifndef _WIN32

[[gnu::pure]]
bool
IsLocalSocket(const struct mpd_settings &settings) noexcept;

#endif // _WIN32

[[gnu::pure]]
std::string
GetName(const struct mpd_settings &settings) noexcept;

} // namespace MPD
