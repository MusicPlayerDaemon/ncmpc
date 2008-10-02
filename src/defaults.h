/* ncmpc
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
 * This project's homepage is: http://www.musicpd.org
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DEFAULTS_H
#define DEFAULTS_H

/* mpd crossfade time [s] */
#define DEFAULT_CROSSFADE_TIME 10

/* screen list */
#define DEFAULT_SCREEN_LIST "playlist browse"

/* song format - list window */
#define DEFAULT_LIST_FORMAT "%name%|[%artist% - ]%title%|%shortfile%"

/* song format - status window */
#define DEFAULT_STATUS_FORMAT "[%artist% - ]%title%|%shortfile%"

#define DEFAULT_LYRICS_TIMEOUT 100

#define DEFAULT_SCROLL TRUE
#define DEFAULT_SCROLL_SEP " *** "

#endif
