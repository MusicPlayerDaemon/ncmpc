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

#ifndef COMMAND_H
#define COMMAND_H

#include "config.h"
#include "Compiler.h"

#include <stddef.h>

#ifndef NCMPC_MINI
#include <stdio.h>
#endif

#define MAX_COMMAND_KEYS 3

/* commands */
typedef enum {
	CMD_SCREEN_KEYDEF,
	CMD_QUIT,

	/* movement */
	CMD_LIST_PREVIOUS,
	CMD_LIST_NEXT,
	CMD_LIST_TOP,
	CMD_LIST_MIDDLE,
	CMD_LIST_BOTTOM,
	CMD_LIST_FIRST,
	CMD_LIST_LAST,
	CMD_LIST_PREVIOUS_PAGE,
	CMD_LIST_NEXT_PAGE,
	CMD_LIST_RANGE_SELECT,
	CMD_LIST_SCROLL_DOWN_LINE,
	CMD_LIST_SCROLL_UP_LINE,
	CMD_LIST_SCROLL_DOWN_HALF,
	CMD_LIST_SCROLL_UP_HALF,
	CMD_SELECT_PLAYING,

	/* basic screens */
	CMD_SCREEN_HELP,
	CMD_SCREEN_PLAY,
	CMD_SCREEN_FILE,

	/* player commands */
	CMD_PLAY,
	CMD_PAUSE,
	CMD_STOP,
	CMD_CROP,
	CMD_TRACK_NEXT,
	CMD_TRACK_PREVIOUS,
	CMD_SEEK_FORWARD,
	CMD_SEEK_BACKWARD,
	CMD_VOLUME_UP,
	CMD_VOLUME_DOWN,
	CMD_SELECT,
	CMD_SELECT_ALL,
	CMD_DELETE,
	CMD_SHUFFLE,
	CMD_CLEAR,
	CMD_REPEAT,
	CMD_RANDOM,
	CMD_SINGLE,
	CMD_CONSUME,
	CMD_CROSSFADE,
	CMD_DB_UPDATE,
	CMD_SAVE_PLAYLIST,
	CMD_ADD,
	CMD_GO_ROOT_DIRECTORY,
	CMD_GO_PARENT_DIRECTORY,
	CMD_LOCATE,

	/* lists */
	CMD_LIST_MOVE_UP,
	CMD_LIST_MOVE_DOWN,
	CMD_SCREEN_UPDATE,

	/* ncmpc options */
	CMD_TOGGLE_FIND_WRAP,
	CMD_TOGGLE_AUTOCENTER,

	/* change screen */
	CMD_SCREEN_NEXT,
	CMD_SCREEN_PREVIOUS,
	CMD_SCREEN_SWAP,

	/* find */
	CMD_LIST_FIND,
	CMD_LIST_FIND_NEXT,
	CMD_LIST_RFIND,
	CMD_LIST_RFIND_NEXT,
	CMD_LIST_JUMP,

	/* extra screens */
	CMD_SCREEN_ARTIST,
	CMD_SCREEN_SEARCH,
	CMD_SEARCH_MODE,
	CMD_SCREEN_SONG,
	CMD_SCREEN_LYRICS,
	CMD_INTERRUPT,
	CMD_LYRICS_UPDATE,
	CMD_EDIT,
	CMD_SCREEN_OUTPUTS,
	CMD_SCREEN_CHAT,

	CMD_NONE,
} command_t;


#ifndef NCMPC_MINI
/* command definition flags */
#define COMMAND_KEY_MODIFIED  0x01
#define COMMAND_KEY_CONFLICT  0x02
#endif

/* write key bindings flags */
#define KEYDEF_WRITE_HEADER  0x01
#define KEYDEF_WRITE_ALL     0x02
#define KEYDEF_COMMENT_ALL   0x04

typedef struct  {
	int keys[MAX_COMMAND_KEYS];
	char flags;
	command_t command;
	const char *name;
	const char *description;
} command_definition_t;

#ifdef ENABLE_KEYDEF_SCREEN
command_definition_t *get_command_definitions();
size_t get_cmds_max_name_width(command_definition_t *cmds);
#endif

gcc_pure
command_t
find_key_command(int key, const command_definition_t *cmds);

void command_dump_keys();

#ifndef NCMPC_MINI

/**
 * @return true on success, false on error
 */
bool
check_key_bindings(command_definition_t *cmds, char *buf, size_t size);

/**
 * @return true on success, false on error
 */
bool
write_key_bindings(FILE *f, int all);

#endif

gcc_pure
const char *key2str(int key);

gcc_pure
const char *get_key_description(command_t command);

gcc_pure
const char *get_key_command_name(command_t command);

gcc_pure
const char *get_key_names(command_t command, bool all);

gcc_pure
command_t get_key_command(int key);

gcc_pure
command_t
get_key_command_from_name(const char *name);

/**
 * @return true on success, false on error
 */
bool
assign_keys(command_t command, int keys[MAX_COMMAND_KEYS]);

#endif
