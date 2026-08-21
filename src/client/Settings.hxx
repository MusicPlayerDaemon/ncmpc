// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "net/Features.hxx" // for HAVE_UN

#include <string>

struct mpd_settings;

namespace MPD {

#ifdef HAVE_UN

[[gnu::pure]]
bool
IsLocalSocket(const struct mpd_settings &settings) noexcept;

#endif // HAVE_UN

[[gnu::pure]]
std::string
GetName(const struct mpd_settings &settings) noexcept;

} // namespace MPD
