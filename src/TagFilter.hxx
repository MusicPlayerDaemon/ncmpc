/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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

#ifndef NCMPC_TAG_FILTER_HXX
#define NCMPC_TAG_FILTER_HXX

#include "util/Compiler.h"

#include <mpd/tag.h>

#include <string>
#include <forward_list>

class ScreenManager;

using TagFilter = std::forward_list<std::pair<enum mpd_tag_type, std::string>>;

gcc_pure
const char *
FindTag(const TagFilter &filter, enum mpd_tag_type tag) noexcept;

void
AddConstraints(struct mpd_connection *connection,
	       const TagFilter &filter) noexcept;

#endif
