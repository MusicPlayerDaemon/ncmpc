// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef CONFIG_FILE_HXX
#define CONFIG_FILE_HXX

#include <string>

std::string
MakeKeysPath();

#ifndef _WIN32
std::string
GetHomeConfigPath() noexcept;
#endif

std::string
GetUserConfigPath() noexcept;

std::string
GetSystemConfigPath() noexcept;

void
read_configuration();

#endif
