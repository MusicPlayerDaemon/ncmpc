// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Settings.hxx"

#include <fmt/format.h>
#include <mpd/client.h>

using std::string_view_literals::operator""sv;

namespace MPD {

static constexpr bool
is_local_socket(const char *host) noexcept
{
#ifdef _WIN32
	return false;
#elifdef __linux__
	// Linux has abstract sockets (starting with '@')
	return *host == '/' || *host == '@';
#else
	return *host == '/';
#endif
}

#ifndef _WIN32

[[gnu::pure]]
bool
IsLocalSocket(const struct mpd_settings &settings) noexcept
{
	const char *host = mpd_settings_get_host(&settings);
	return host != nullptr && is_local_socket(host);
}

#endif

std::string
GetName(const struct mpd_settings &settings) noexcept
{
	const char *host = mpd_settings_get_host(&settings);
	if (host == nullptr)
		host = "unknown";

	if (host[0] == '/' || host[0] == '@')
		return host;

	unsigned port = mpd_settings_get_port(&settings);
	if (port == 0 || port == 6600)
		return host;

	return fmt::format("{}:{}"sv, host, port);
}

} // namespace MPD
