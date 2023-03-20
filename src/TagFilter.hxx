// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_TAG_FILTER_HXX
#define NCMPC_TAG_FILTER_HXX

#include <mpd/tag.h>

#include <string>
#include <forward_list>

class ScreenManager;

using TagFilter = std::forward_list<std::pair<enum mpd_tag_type, std::string>>;

[[gnu::pure]]
const char *
FindTag(const TagFilter &filter, enum mpd_tag_type tag) noexcept;

void
AddConstraints(struct mpd_connection *connection,
	       const TagFilter &filter) noexcept;

[[gnu::pure]]
std::string
ToString(const TagFilter &filter) noexcept;

#endif
