/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#ifndef DEFAULTS_H
#define DEFAULTS_H

/* mpd crossfade time [s] */
#define DEFAULT_CROSSFADE_TIME 10

/* screen list */
#define DEFAULT_SCREEN_LIST {"playlist", "browse"}

/* song format - list window */
#define DEFAULT_LIST_FORMAT "%name%|[[%artist%|%performer%|%composer%|%albumartist%] - ][%title%|%shortfile%]"

/* song format - status window */
#define DEFAULT_STATUS_FORMAT "[[%artist%|%performer%|%composer%|%albumartist%] - ][%title%|%shortfile%]"

#define DEFAULT_LYRICS_TIMEOUT 100

#define DEFAULT_SCROLL true
#define DEFAULT_SCROLL_SEP " *** "

#endif
