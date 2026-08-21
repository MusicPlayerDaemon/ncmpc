// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "mpd/client.h"

#include <string>

struct mpd_settings;

namespace MPD {

using SharedSettings = std::shared_ptr<struct mpd_settings>;

inline SharedSettings
NewSettings(const char *host, unsigned port, unsigned timeout_ms,
	    const char *reserved, const char *password) noexcept
{
	return SharedSettings{
		mpd_settings_new(host, port, timeout_ms,
				 reserved, password),
		mpd_settings_free,
	};
}

} // namespace MPD
