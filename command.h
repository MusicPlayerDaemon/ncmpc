#ifndef COMMAND_H
#define COMMAND_H

#define MAX_COMMAND_KEYS 3

typedef enum
{
  CMD_NONE = 0,
  CMD_PLAY,
  CMD_SELECT,
  CMD_PAUSE,
  CMD_STOP,
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
  CMD_VOLUME_UP,
  CMD_VOLUME_DOWN,
  CMD_SAVE_PLAYLIST,
  CMD_TOGGLE_FIND_WRAP,
  CMD_TOGGLE_AUTOCENTER,
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
  CMD_SCREEN_UPDATE,
  CMD_SCREEN_PREVIOUS,
  CMD_SCREEN_NEXT,
  CMD_SCREEN_PLAY,
  CMD_SCREEN_FILE,
  CMD_SCREEN_SEARCH,
  CMD_SCREEN_KEYDEF,
  CMD_SCREEN_HELP,
  CMD_QUIT
} command_t;

typedef struct 
{
  int keys[MAX_COMMAND_KEYS];
  command_t command;
  char *name;
  char *description;
} command_definition_t;

command_definition_t *get_command_definitions(void);
command_t find_key_command(int key, command_definition_t *cmds);

void command_dump_keys(void);
int  check_key_bindings(void);
int  write_key_bindings(FILE *f);

char *key2str(int key);
char *get_key_description(command_t command);
char *get_key_command_name(command_t command);
char *get_key_names(command_t command, int all);
command_t get_key_command(int key);
command_t get_key_command_from_name(char *name);
int assign_keys(command_t command, int keys[MAX_COMMAND_KEYS]);

command_t get_keyboard_command(void);

#endif
