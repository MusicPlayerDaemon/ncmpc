
typedef enum
{
  CMD_NONE = 0,
  CMD_PLAY,
  CMD_SELECT,
  CMD_PAUSE,
  CMD_STOP,
  CMD_TRACK_NEXT,
  CMD_TRACK_PREVIOUS,
  CMD_SHUFFLE,
  CMD_RANDOM,
  CMD_CLEAR,
  CMD_DELETE,
  CMD_REPEAT,
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
  CMD_SCREEN_HELP,
  CMD_QUIT
} command_t;

typedef struct 
{
  int keys[3];
  command_t command;
  char *description;
} command_definition_t;


void command_dump_keys(void);
char *command_get_keys(command_t command);

command_t get_keyboard_command(void);
