#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "command.h"

#undef DEBUG_KEYS

#ifdef DEBUG_KEYS
#define DK(x) x
#else
#define DK(x)
#endif

#define BS   KEY_BACKSPACE
#define DEL  KEY_DC
#define UP   KEY_UP
#define DWN  KEY_DOWN
#define LEFT KEY_LEFT
#define RGHT KEY_RIGHT
#define HOME KEY_HOME
#define END  KEY_END
#define PGDN KEY_NPAGE
#define PGUP KEY_PPAGE
#define TAB  0x09
#define STAB 0x161
#define ESC  0x1B
#define F1   KEY_F(1)
#define F2   KEY_F(2)
#define F3   KEY_F(3)
#define F4   KEY_F(4)
#define F5   KEY_F(5)
#define F6   KEY_F(6)

static command_definition_t cmds[] =
{
  { {  13,   0,   0 }, CMD_PLAY, "play",  
    "Play/Enter directory" },
  { { 'P',   0,   0 }, CMD_PAUSE,"pause", 
    "Pause" },
  { { 's',  BS,   0 }, CMD_STOP, "stop",   
    "Stop" },
  { { '>',   0,   0 }, CMD_TRACK_NEXT, "next", 
    "Next track" },
  { { '<',   0,   0 }, CMD_TRACK_PREVIOUS, "prev", 
    "Previous track" },

  { { '+', RGHT,  0 }, CMD_VOLUME_UP, "volume-up", 
    "Increase volume" },
  { { '-', LEFT,  0 }, CMD_VOLUME_DOWN, "volume-down", 
    "Decrease volume" },

  { { 'w',   0,   0 }, CMD_TOGGLE_FIND_WRAP,  "wrap-mode", 
    "Toggle find mode" },
  { { 'U',   0,   0 }, CMD_TOGGLE_AUTOCENTER, "autocenter-mode", 
    "Toggle auto center mode" },

  { { ' ',  'a',   0 }, CMD_SELECT, "select", 
    "Select/deselect song in playlist" },
  { { DEL,  'd',  0 }, CMD_DELETE, "delete",
    "Delete song from playlist" },
  { { 'Z',   0,   0 }, CMD_SHUFFLE, "shuffle",
    "Shuffle playlist" },
  { { 'c',   0,   0 }, CMD_CLEAR, "clear",
    "Clear playlist" },
  { { 'r',   0,   0 }, CMD_REPEAT, "repeat",
    "Toggle repeat mode" },
  { { 'z',   0,   0 }, CMD_RANDOM, "random",
    "Toggle random mode" },
  { { 'S',   0,   0 }, CMD_SAVE_PLAYLIST, "save",
    "Save playlist" },

  { {  UP,  ',',   0 }, CMD_LIST_PREVIOUS,      "up",
    "Move cursor up" },
  { { DWN,  '.',   0 }, CMD_LIST_NEXT,          "down",
    "Move cursor down" },
  { { HOME, 0x01, 0 }, CMD_LIST_FIRST,          "home",
    "Home" },
  { { END,  0x05, 0 }, CMD_LIST_LAST,           "end",
    "End" },
  { { PGUP, 'A',   0 }, CMD_LIST_PREVIOUS_PAGE, "pgup",
    "Page up" },
  { { PGDN, 'B',   0 }, CMD_LIST_NEXT_PAGE,     "pgdn", 
    "Page down" },
  { { '/',   0,   0 }, CMD_LIST_FIND,           "find",
    "Forward find" },
  { { 'n',   0,   0 }, CMD_LIST_FIND_NEXT,      "find-next",
    "Forward find next" },
  { { '?',   0,   0 }, CMD_LIST_RFIND,          "rfind",
    "Backward find" },
  { { 'p',   0,   0 }, CMD_LIST_RFIND_NEXT,     "rfind-next",
    "Backward find previous" },


  { { TAB,   0,   0 }, CMD_SCREEN_NEXT,     "screen-next",
    "Next screen" },

  { { STAB,  0,   0 }, CMD_SCREEN_PREVIOUS, "screen-prev",
    "Previous screen" },

  { { '1', F1, 'h' }, CMD_SCREEN_HELP,      "screen-help",
    "Help screen" },
  { { '2', F2,  0 }, CMD_SCREEN_PLAY,      "screen-playlist",
    "Playlist screen" },
  { { '3', F3,  0 }, CMD_SCREEN_FILE,      "screen-browse",
    "Browse screen" },
  { {'u',   0,   0 }, CMD_SCREEN_UPDATE,    "update",
    "Update screen" },

  { { 'q',  0,   0 }, CMD_QUIT,   "quit",
    "Quit " PACKAGE },  

  { { -1,  -1,  -1 }, CMD_NONE, NULL, NULL }
};

char *
key2str(int key)
{
  static char buf[2];

  buf[0] = 0;
  switch(key)
    {
    case 0:
      return "Undefined";
    case ' ':
      return "Space";
    case 13:
      return "Enter";
    case BS:
      return "Backspace";
    case DEL:
      return "Delete";
    case UP: 
      return "Up";
    case DWN:
      return "Down";
    case LEFT:
      return "Left";
    case RGHT:
      return "Right";
    case HOME:
      return "Home";
    case END:
      return "End";
    case PGDN:
      return "PageDown";
    case PGUP:
      return "PageUp";
    case TAB: 
      return "Tab";
    case STAB:
      return "Shift+Tab";
    case ESC:
      return "Esc";
    case F1:
      return "F1";
    case F2:
      return "F2";
    case F3:
      return "F3";
    case F4:
      return "F4";
    case F5:
      return "F5";
    case F6:
      return "F6";
    default:
      snprintf(buf, 2, "%c", key);
    }
  return buf;
}

void
command_dump_keys(void)
{
  int i;

  i=0;
  while( cmds[i].description )
    {
      if( cmds[i].command != CMD_NONE )
	printf(" %20s : %s\n", get_key_names(cmds[i].command,1),cmds[i].name); 
      i++;
    }
}

char *
get_key_names(command_t command, int all)
{
  int i;
  
  i=0;
  while( cmds[i].description )
    {
      if( cmds[i].command == command )
	{
	  int j;
	  static char keystr[80];

	  strncpy(keystr, key2str(cmds[i].keys[0]), 80);
	  if( !all )
	    return keystr;
	  j=1;
	  while( j<MAX_COMMAND_KEYS && cmds[i].keys[j]>0 )
	    {
	      strcat(keystr, " ");
	      strcat(keystr, key2str(cmds[i].keys[j]));
	      j++;
	    }
	  return keystr;
 	}
      i++;
    }
  return NULL;
}

char *
get_key_description(command_t command)
{
  int i;

  i=0;
  while( cmds[i].description )
    {
      if( cmds[i].command == command )
	return cmds[i].description;
      i++;
    }
  return NULL;
}

char *
get_key_command_name(command_t command)
{
  int i;

  i=0;
  while( cmds[i].name )
    {
      if( cmds[i].command == command )
	return cmds[i].name;
      i++;
    }
  return NULL;
}

command_t 
get_key_command_from_name(char *name)
{
  int i;

  i=0;
  while( cmds[i].name )
    {
      if( strcmp(name, cmds[i].name) == 0 )
	return cmds[i].command;
      i++;
    }
  return CMD_NONE;
}

command_t 
get_key_command(int key)
{
  int i;

  i=0;
  while( cmds[i].name )
    {
      if( cmds[i].keys[0] == key || 
	  cmds[i].keys[1] == key ||
	  cmds[i].keys[2] == key )
	return cmds[i].command;
      i++;
    }
  return CMD_NONE;
}


command_t
get_keyboard_command(void)
{
  int key;

  key = wgetch(stdscr);

  if( key==ERR )
    return CMD_NONE;

  DK(fprintf(stderr, "key = 0x%02X\t", key));

  return get_key_command(key);
}

int
assign_keys(command_t command, int keys[MAX_COMMAND_KEYS])
{
 int i;

  i=0;
  while( cmds[i].name )
    {
      if( cmds[i].command == command )
	{
	  memcpy(cmds[i].keys, keys, sizeof(int)*MAX_COMMAND_KEYS);
	  return 0;
	}
      i++;
    }
  return -1;
}

int 
check_key_bindings(void)
{
  int i;
  int retval = 0;
  
  i=0;
  while( cmds[i].name )
    {
      int j;
      command_t cmd;

      for(j=0; j<MAX_COMMAND_KEYS; j++)
	if( cmds[i].keys[j] && 
	    (cmd=get_key_command(cmds[i].keys[j])) != cmds[i].command )
	  {
	    fprintf(stderr, "Error: Key %s assigned to %s and %s !!!\n",
		    key2str(cmds[i].keys[j]),
		    get_key_command_name(cmds[i].command),
		    get_key_command_name(cmd));
	    retval = -1;
	  }
      i++;
    }
  return retval;
}

int
write_key_bindings(FILE *f)
{
  int i,j;

  i=0;
  while( cmds[i].name && !ferror(f) )
    {
      fprintf(f, "# %s\n", cmds[i].description);
      fprintf(f, "key %s = ", cmds[i].name);
      for(j=0; j<MAX_COMMAND_KEYS; j++)
	{
	  if( j && cmds[i].keys[j] )
	    fprintf(f, ",  ");
	  if( !j || cmds[i].keys[j] )
	    {
	      if( cmds[i].keys[j]<256 && (isalpha(cmds[i].keys[j]) || 
					  isdigit(cmds[i].keys[j])) )
		fprintf(f, "\'%c\'", cmds[i].keys[j]);
	      else
		fprintf(f, "%d", cmds[i].keys[j]);
	    }
	}
      fprintf(f,"\n\n");
      i++;
    }
  return ferror(f);
}
