// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
