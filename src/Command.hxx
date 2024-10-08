// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

#include <cstddef>

/* commands */
enum class Command : unsigned {
#ifdef ENABLE_KEYDEF_SCREEN
	SCREEN_KEYDEF,
#endif
	QUIT,

	/* movement */
	LIST_PREVIOUS,
	LIST_NEXT,
	LIST_TOP,
	LIST_MIDDLE,
	LIST_BOTTOM,
	LIST_FIRST,
	LIST_LAST,
	LIST_PREVIOUS_PAGE,
	LIST_NEXT_PAGE,
	LIST_RANGE_SELECT,
	LIST_SCROLL_DOWN_LINE,
	LIST_SCROLL_UP_LINE,
	LIST_SCROLL_DOWN_HALF,
	LIST_SCROLL_UP_HALF,
	SELECT_PLAYING,

	/* basic screens */
	SCREEN_HELP,
	SCREEN_PLAY,
	SCREEN_FILE,

	/* player commands */
	PLAY,
	PAUSE,
	STOP,
	CROP,
	TRACK_NEXT,
	TRACK_PREVIOUS,
	SEEK_FORWARD,
	SEEK_BACKWARD,
	VOLUME_UP,
	VOLUME_DOWN,
	SELECT,
	SELECT_ALL,
	DELETE,
	SHUFFLE,
	CLEAR,
	REPEAT,
	RANDOM,
	SINGLE,
	CONSUME,
	CROSSFADE,
	DB_UPDATE,
	SAVE_PLAYLIST,
	ADD,
	GO_ROOT_DIRECTORY,
	GO_PARENT_DIRECTORY,
	LOCATE,

	/* lists */
	LIST_MOVE_UP,
	LIST_MOVE_DOWN,
	SCREEN_UPDATE,

	/* ncmpc options */
	TOGGLE_FIND_WRAP,
	TOGGLE_AUTOCENTER,

	/* change screen */
	SCREEN_NEXT,
	SCREEN_PREVIOUS,
	SCREEN_SWAP,

	/* find */
	LIST_FIND,
	LIST_FIND_NEXT,
	LIST_RFIND,
	LIST_RFIND_NEXT,
	LIST_JUMP,

	/* extra screens */
#ifdef ENABLE_LIBRARY_PAGE
	LIBRARY_PAGE,
#endif
#ifdef ENABLE_SEARCH_SCREEN
	SCREEN_SEARCH,
	SEARCH_MODE,
#endif
#ifdef ENABLE_PLAYLIST_EDITOR
	PLAYLIST_EDITOR_PAGE,
#endif
#ifdef ENABLE_SONG_SCREEN
	SCREEN_SONG,
#endif
#ifdef ENABLE_LYRICS_SCREEN
	SCREEN_LYRICS,
	INTERRUPT,
	LYRICS_UPDATE,
#endif
#if defined(ENABLE_LYRICS_SCREEN) || defined(ENABLE_PLAYLIST_EDITOR)
	EDIT,
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	SCREEN_OUTPUTS,
#endif
#ifdef ENABLE_CHAT_SCREEN
	SCREEN_CHAT,
#endif

	NONE,
};

typedef struct  {
	const char *name;
	const char *description;
} command_definition_t;

[[gnu::const]]
const command_definition_t *
get_command_definitions() noexcept;

[[gnu::const]]
std::size_t
get_cmds_max_name_width() noexcept;

[[gnu::pure]]
const char *
get_key_description(Command command) noexcept;

[[gnu::pure]]
const char *
get_key_command_name(Command command) noexcept;

[[gnu::pure]]
Command
get_key_command_from_name(const char *name) noexcept;
