/* 
 * $Id: command.c,v 1.9 2004/03/17 14:59:32 kalle Exp $ 
 *
 */
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
#define STAB 0x5A
#define ESC  0x1B
#define F1   KEY_F(1)
#define F2   KEY_F(2)
#define F3   KEY_F(3)
#define F4   KEY_F(4)
#define F5   KEY_F(5)
#define F6   KEY_F(6)

static command_definition_t cmds[] =
{
  { {  13,   0,   0 }, CMD_PLAY, "Play/Enter directory" },
  { { 'P',   0,   0 }, CMD_PAUSE, "Pause" },
  { {  BS, ESC,   0 }, CMD_STOP, "Stop" },
  { { '>',   0,   0 }, CMD_TRACK_NEXT, "Next song" },
  { { '<',   0,   0 }, CMD_TRACK_PREVIOUS, "Previous song" },

  { { '+', RGHT,  0 }, CMD_VOLUME_UP, "Increase volume" },
  { { '-', LEFT,  0 }, CMD_VOLUME_DOWN, "Decrease volume" },

  { { 'w',   0,   0 }, CMD_TOGGLE_FIND_WRAP, "Toggle find mode" },

  { { ' ',   0,   0 }, CMD_SELECT, "Select/deselect song in playlist" },
  { { DEL,   0,   0 }, CMD_DELETE, "Delete song from playlist" },
  { { 's',   0,   0 }, CMD_SHUFFLE, "Shuffle playlist" },
  { { 'c',   0,   0 }, CMD_CLEAR, "Clear playlist" },
  { { 'r',   0,   0 }, CMD_REPEAT, "Toggle repeat mode" },
  { { 'z',   0,   0 }, CMD_RANDOM, "Toggle random mode" },
  { { 'S',   0,   0 }, CMD_SAVE_PLAYLIST, "Save playlist" },

  { {  UP,   0,   0 }, CMD_LIST_PREVIOUS,      "Move: Up" },
  { { DWN,   0,   0 }, CMD_LIST_NEXT,          "Move: Down" },
  { { HOME,  0,   0 }, CMD_LIST_FIRST,         "Move: Home" },
  { { END,   0,   0 }, CMD_LIST_LAST,          "Move: End" },
  { { PGUP,  0,   0 }, CMD_LIST_PREVIOUS_PAGE, "Move: Page Up" },
  { { PGDN,  0,   0 }, CMD_LIST_NEXT_PAGE,     "Move: Page Down" },
  { { '/',   0,   0 }, CMD_LIST_FIND,          "Forward Find" },
  { { 'n',   0,   0 }, CMD_LIST_FIND_NEXT,     "Forward Find Next" },
  { { '?',   0,   0 }, CMD_LIST_RFIND,         "Backward Find" },
  { { 'p',   0,   0 }, CMD_LIST_RFIND_NEXT,    "Backward Find Previous" },

  { { TAB,   0,   0 }, CMD_SCREEN_NEXT,   "Next screen" },
  { { STAB,  0,   0 }, CMD_SCREEN_PREVIOUS, "Previous screen" },
  { { F1, '1', 'h' }, CMD_SCREEN_HELP,   "Help screen" },
  { { F2, '2',   0 }, CMD_SCREEN_PLAY,   "Playlist screen" },
  { { F3, '3',   0 }, CMD_SCREEN_FILE,   "Browse screen" },
  //  { { F4, '4',   0 }, CMD_SCREEN_SEARCH, "Search screen" },

  { { 'q',  0,   0 }, CMD_QUIT,   "Quit " PACKAGE },  

  { { -1,  -1,  -1 }, CMD_NONE, NULL }
};

char *
key2str(int key)
{
  static char buf[2];

  buf[0] = 0;
  switch(key)
    {
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
	{
	  int j;
	  char keystr[80];

	  strcpy(keystr, key2str(cmds[i].keys[0]));
	  j=1;
	  while( j<3 && cmds[i].keys[j]>0 )
	    {
	      strcat(keystr, " ");
	      strcat(keystr, key2str(cmds[i].keys[j]));
	      j++;
	    }
	  printf(" %20s : %s\n", keystr, cmds[i].description);
 
	}
      i++;
    }
}

char *
command_get_keys(command_t command)
{
  int i;

  i=0;
  while( cmds[i].description )
    {
      if( cmds[i].command == command )
	{
	  int j;
	  static char keystr[80];

	  strcpy(keystr, key2str(cmds[i].keys[0]));
	  j=1;
	  while( j<3 && cmds[i].keys[j]>0 )
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

command_t
get_keyboard_command(void)
{
  int i;
  int key;

  key = wgetch(stdscr);

  if( key==ERR )
    return CMD_NONE;

  DK(fprintf(stderr, "key = 0x%02X\t", key));

  //  if( isalpha(key) )
  //    key=tolower(key);

  i=0;
  while( cmds[i].description )
    {
      if( cmds[i].keys[0] == key || 
	  cmds[i].keys[1] == key ||
	  cmds[i].keys[2] == key )
	{
	  DK(fprintf(stderr, "Match - %s\n", cmds[i].description));
	  return cmds[i].command;
	}
      i++;
    } 
  DK(fprintf(stderr, "NO MATCH\n"));
  return CMD_NONE;
}
