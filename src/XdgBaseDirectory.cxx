// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "XdgBaseDirectory.hxx"
#include "io/Path.hxx"

#include <stdlib.h>
#include <sys/stat.h>

[[gnu::pure]]
static bool
IsDirectory(const char *path) noexcept
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

[[gnu::const]]
static const char *
GetHomeDirectory() noexcept
{
	return getenv("HOME");
}

[[gnu::const]]
static std::string
GetHomeConfigDirectory() noexcept
{
	const char *config_home = getenv("XDG_CONFIG_HOME");
	if (config_home != nullptr && *config_home != 0)
		return config_home;

	const char *home = GetHomeDirectory();
	if (home != nullptr)
		return BuildPath(home, ".config");

	return {};
}

std::string
GetHomeConfigDirectory(std::string_view package) noexcept
{
	const auto dir = GetHomeConfigDirectory();
	if (dir.empty())
		return {};

	return BuildPath(dir, package);
}

std::string
MakeUserConfigPath(std::string_view package,
		   std::string_view filename) noexcept
{
	const auto directory = GetHomeConfigDirectory(package);
	if (directory.empty())
		return {};

	return IsDirectory(directory.c_str()) ||
		mkdir(directory.c_str(), 0777) == 0
		? BuildPath(directory, filename)
		: std::string();
}

[[gnu::const]]
static std::string
GetHomeCacheDirectory() noexcept
{
	const char *cache_home = getenv("XDG_CACHE_HOME");
	if (cache_home != nullptr && *cache_home != 0)
		return cache_home;

	const char *home = GetHomeDirectory();
	if (home != nullptr)
		return BuildPath(home, ".cache");

	return {};
}

std::string
GetHomeCacheDirectory(std::string_view package) noexcept
{
	const auto dir = GetHomeCacheDirectory();
	if (dir.empty())
		return {};

	return BuildPath(dir, package);
}
