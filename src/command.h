#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>
#include <stdio.h>
#include <ncurses.h>

#define MAX_COMMAND_KEYS 3

/* commands */
typedef enum {
	CMD_NONE = 0,
	CMD_PLAY,
	CMD_SELECT,
	CMD_SELECT_ALL,
	CMD_PAUSE,
	CMD_STOP,
	CMD_CROP,
	CMD_TRACK_NEXT,
	CMD_TRACK_PREVIOUS,
	CMD_SEEK_FORWARD,
	CMD_SEEK_BACKWARD,
	CMD_SHUFFLE,
	CMD_RANDOM,
	CMD_CLEAR,
	CMD_DELETE,
	CMD_REPEAT,
	CMD_CROSSFADE,
	CMD_DB_UPDATE,
	CMD_VOLUME_UP,
	CMD_VOLUME_DOWN,
	CMD_ADD,
	CMD_SAVE_PLAYLIST,
	CMD_TOGGLE_FIND_WRAP,
	CMD_TOGGLE_AUTOCENTER,
	CMD_SEARCH_MODE,
	CMD_LIST_PREVIOUS,
	CMD_LIST_NEXT,
	CMD_LIST_FIRST,
	CMD_LIST_LAST,
	CMD_LIST_NEXT_PAGE,
	CMD_LIST_PREVIOUS_PAGE,
	CMD_LIST_FIND,
	CMD_LIST_FIND_NEXT,
	CMD_LIST_RFIND,
	CMD_LIST_RFIND_NEXT,
	CMD_LIST_MOVE_UP,
	CMD_LIST_MOVE_DOWN,
	CMD_MOUSE_EVENT,
	CMD_SCREEN_UPDATE,
	CMD_SCREEN_PREVIOUS,
	CMD_SCREEN_NEXT,
	CMD_SCREEN_PLAY,
	CMD_SCREEN_FILE,
	CMD_SCREEN_ARTIST,
	CMD_SCREEN_SEARCH,
	CMD_SCREEN_KEYDEF,
	CMD_SCREEN_HELP,
	CMD_SCREEN_LYRICS,
	CMD_LYRICS_UPDATE,
	CMD_INTERRUPT,
	CMD_GO_ROOT_DIRECTORY,
	CMD_GO_PARENT_DIRECTORY,
	CMD_QUIT
} command_t;


/* command definition flags */
#define COMMAND_KEY_MODIFIED  0x01
#define COMMAND_KEY_CONFLICT  0x02

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

command_definition_t *get_command_definitions(void);
command_t find_key_command(int key, command_definition_t *cmds);

void command_dump_keys(void);
int check_key_bindings(command_definition_t *cmds, char *buf, size_t size);
int write_key_bindings(FILE *f, int all);

const char *key2str(int key);
const char *get_key_description(command_t command);
const char *get_key_command_name(command_t command);
const char *get_key_names(command_t command, int all);
command_t get_key_command(int key);
command_t get_key_command_from_name(char *name);
int assign_keys(command_t command, int keys[MAX_COMMAND_KEYS]);

command_t get_keyboard_command(void);

#endif
