// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ConfigFile.hxx"
#include "ConfigParser.hxx"
#include "config.h"
#include "Options.hxx"
#include "io/Path.hxx"

#include <sys/stat.h>

#ifdef _WIN32
#include <glib.h>
#else
#include "XdgBaseDirectory.hxx"
#endif

#ifdef _WIN32
#define CONFIG_FILENAME "ncmpc.conf"
#define KEYS_FILENAME "keys.conf"
#else
#define CONFIG_FILENAME "config"
#define KEYS_FILENAME "keys"
#endif

[[gnu::pure]]
static bool
IsFile(const char *path) noexcept
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

std::string
MakeKeysPath()
{
	return MakeUserConfigPath(KEYS_FILENAME);
}

#ifndef _WIN32

std::string
GetHomeConfigPath() noexcept
{
	const char *home = GetHomeDirectory();
	if (home == nullptr)
		return {};

	return BuildPath(home, "." PACKAGE, CONFIG_FILENAME);
}

#endif

std::string
GetUserConfigPath() noexcept
{
	const auto dir = GetHomeConfigDirectory();
	if (dir.empty())
		return {};

	return BuildPath(dir, PACKAGE, CONFIG_FILENAME);
}

std::string
GetSystemConfigPath() noexcept
{
#ifdef _WIN32
	const gchar* const *system_data_dirs;

	for (system_data_dirs = g_get_system_config_dirs (); *system_data_dirs != nullptr; system_data_dirs++)
	{
		auto path = BuildPath(*system_data_dirs, PACKAGE, CONFIG_FILENAME);
		if (IsFile(path.c_str()))
			return path;
	}
	return {};
#else
	return BuildPath(SYSCONFDIR, PACKAGE, CONFIG_FILENAME);
#endif
}

#ifndef _WIN32

[[gnu::pure]]
static std::string
GetHomeKeysPath() noexcept
{
	const char *home = GetHomeDirectory();
	if (home == nullptr)
		return {};

	return BuildPath(home, "." PACKAGE, KEYS_FILENAME);
}

#endif

[[gnu::pure]]
static std::string
GetUserKeysPath() noexcept
{
	const auto dir = GetHomeConfigDirectory();
	if (dir.empty())
		return {};

	return BuildPath(dir, PACKAGE, KEYS_FILENAME);
}

[[gnu::pure]]
static std::string
GetSystemKeysPath() noexcept
{
#ifdef _WIN32
	const gchar* const *system_data_dirs;

	for (system_data_dirs = g_get_system_config_dirs (); *system_data_dirs != nullptr; system_data_dirs++)
	{
		auto path = BuildPath(*system_data_dirs, PACKAGE, KEYS_FILENAME);
		if (IsFile(pathname.c_str()))
			return path;
	}
	return {}
#else
	return BuildPath(SYSCONFDIR, PACKAGE, KEYS_FILENAME);
#endif
}

static std::string
find_config_file() noexcept
{
	/* check for command line configuration file */
	if (!options.config_file.empty())
		return options.config_file;

	/* check for user configuration ~/.config/ncmpc/config */
	auto filename = GetUserConfigPath();
	if (!filename.empty() && IsFile(filename.c_str()))
		return filename;

#ifndef _WIN32
	/* check for user configuration ~/.ncmpc/config */
	filename = GetHomeConfigPath();
	if (!filename.empty() && IsFile(filename.c_str()))
		return filename;
#endif

	/* check for  global configuration SYSCONFDIR/ncmpc/config */
	filename = GetSystemConfigPath();
	if (IsFile(filename.c_str()))
		return filename;

	return {};
}

static std::string
find_keys_file() noexcept
{
	/* check for command line key binding file */
	if (!options.key_file.empty())
		return options.key_file;

	/* check for user key bindings ~/.config/ncmpc/keys */
	auto filename = GetUserKeysPath();
	if (!filename.empty() && IsFile(filename.c_str()))
		return filename;

#ifndef _WIN32
	/* check for  user key bindings ~/.ncmpc/keys */
	filename = GetHomeKeysPath();
	if (!filename.empty() && IsFile(filename.c_str()))
		return filename;
#endif

	/* check for  global key bindings SYSCONFDIR/ncmpc/keys */
	filename = GetSystemKeysPath();
	if (IsFile(filename.c_str()))
		return filename;

	return {};
}

void
read_configuration()
{
	/* load configuration */
	auto filename = find_config_file();
	if (!filename.empty())
		ReadConfigFile(filename.c_str());

	/* load key bindings */
	filename = find_keys_file();
	if (!filename.empty())
		ReadConfigFile(filename.c_str());
}
