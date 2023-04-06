// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef XDG_BASE_DIRECTORY_HXX
#define XDG_BASE_DIRECTORY_HXX

#include <string>
#include <string_view>

[[gnu::const]]
std::string
GetHomeConfigDirectory() noexcept;

[[gnu::pure]]
std::string
GetHomeConfigDirectory(std::string_view package) noexcept;

/**
 * Find or create the directory for writing configuration files.
 *
 * @return the absolute path; an empty string indicates that no
 * directory could be created
 */
std::string
MakeUserConfigPath(std::string_view package,
		   std::string_view filename) noexcept;

[[gnu::const]]
std::string
GetHomeCacheDirectory() noexcept;

[[gnu::pure]]
std::string
GetHomeCacheDirectory(std::string_view package) noexcept;

#endif
