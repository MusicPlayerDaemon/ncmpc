/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
