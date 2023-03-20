// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TagFilter.hxx"

#include <mpd/client.h>

const char *
FindTag(const TagFilter &filter, enum mpd_tag_type tag) noexcept
{
	for (const auto &i : filter)
		if (i.first == tag)
			return i.second.c_str();

	return nullptr;
}

void
AddConstraints(struct mpd_connection *connection,
	       const TagFilter &filter) noexcept
{
	for (const auto &i : filter)
		mpd_search_add_tag_constraint(connection,
					      MPD_OPERATOR_DEFAULT,
					      i.first, i.second.c_str());
}

std::string
ToString(const TagFilter &filter) noexcept
{
	std::string result;

	for (const auto &i : filter) {
		if (!result.empty())
			result.insert(0, " - ");
		result.insert(0, i.second);
	}

	return result;
}
