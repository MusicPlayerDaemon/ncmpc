// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef XDG_BASE_DIRECTORY_HXX
#define XDG_BASE_DIRECTORY_HXX

#include <string>
#include <string_view>

[[gnu::pure]]
std::string
GetUserConfigDirectory(std::string_view package) noexcept;

[[gnu::pure]]
std::string
GetUserCacheDirectory(std::string_view package) noexcept;

#endif
