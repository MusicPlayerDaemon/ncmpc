/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "ncmpc.h"
#include "command.h"

#undef DEBUG_KEYS

#ifdef DEBUG_KEYS
#define DK(x) x
#else
#define DK(x)
#endif

extern void screen_resize(void);

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
    N_("Play/Enter directory") },
  { { 'P',   0,   0 }, CMD_PAUSE,"pause", 
    N_("Pause") },
  { { 's',  BS,   0 }, CMD_STOP, "stop",   
    N_("Stop") },
  { { '>',   0,   0 }, CMD_TRACK_NEXT, "next", 
    N_("Next track") },
  { { '<',   0,   0 }, CMD_TRACK_PREVIOUS, "prev", 
    N_("Previous track") },
  { { 'f',   0,   0 }, CMD_SEEK_FORWARD, "seek-forward", 
    N_("Seek forward") },
  { { 'b',   0,   0 }, CMD_SEEK_BACKWARD, "seek-backward", 
    N_("Seek backward") },

  { { '+', RGHT,  0 }, CMD_VOLUME_UP, "volume-up", 
    N_("Increase volume") },
  { { '-', LEFT,  0 }, CMD_VOLUME_DOWN, "volume-down", 
    N_("Decrease volume") },

  { { 'w',   0,   0 }, CMD_TOGGLE_FIND_WRAP,  "wrap-mode", 
    N_("Toggle find mode") },
  { { 'U',   0,   0 }, CMD_TOGGLE_AUTOCENTER, "autocenter-mode", 
    N_("Toggle auto center mode") },

  { { ' ',  'a',   0 }, CMD_SELECT, "select", 
    N_("Select/deselect song in playlist") },
  { { DEL,  'd',  0 }, CMD_DELETE, "delete",
    N_("Delete song from playlist") },
  { { 'Z',   0,   0 }, CMD_SHUFFLE, "shuffle",
    N_("Shuffle playlist") },
  { { 'c',   0,   0 }, CMD_CLEAR, "clear",
    N_("Clear playlist") },
  { { 'r',   0,   0 }, CMD_REPEAT, "repeat",
    N_("Toggle repeat mode") },
  { { 'z',   0,   0 }, CMD_RANDOM, "random",
    N_("Toggle random mode") },
  { { 'x',   0,   0 }, CMD_CROSSFADE, "crossfade",
    N_("Toggle crossfade mode") },
  { { 21,   0,   0 }, CMD_DB_UPDATE,  "db-update",
    N_("Start a music database update") },

  { { 'S',   0,   0 }, CMD_SAVE_PLAYLIST, "save",
    N_("Save playlist") },

  { { 0,  0,   0 }, CMD_LIST_MOVE_UP,     "move-up", 
    N_("Move item up") },
  { { 0,  0,   0 }, CMD_LIST_MOVE_DOWN,   "move-down", 
    N_("Move item down") },

  { {  UP,  ',',   0 }, CMD_LIST_PREVIOUS,      "up",
    N_("Move cursor up") },
  { { DWN,  '.',   0 }, CMD_LIST_NEXT,          "down",
    N_("Move cursor down") },
  { { HOME, 0x01, 0 }, CMD_LIST_FIRST,          "home",
    N_("Home ") },
  { { END,  0x05, 0 }, CMD_LIST_LAST,           "end",
    N_("End ") },
  { { PGUP, 'A',   0 }, CMD_LIST_PREVIOUS_PAGE, "pgup",
    N_("Page up") },
  { { PGDN, 'B',   0 }, CMD_LIST_NEXT_PAGE,     "pgdn", 
    N_("Page down") },
  { { '/',   0,   0 }, CMD_LIST_FIND,           "find",
    N_("Forward find") },
  { { 'n',   0,   0 }, CMD_LIST_FIND_NEXT,      "find-next",
    N_("Forward find next") },
  { { '?',   0,   0 }, CMD_LIST_RFIND,          "rfind",
    N_("Backward find") },
  { { 'p',   0,   0 }, CMD_LIST_RFIND_NEXT,     "rfind-next",
    N_("Backward find previous") },


  { { TAB,   0,   0 }, CMD_SCREEN_NEXT,     "screen-next",
    N_("Next screen") },

  { { STAB,  0,   0 }, CMD_SCREEN_PREVIOUS, "screen-prev",
    N_("Previous screen") },

  { { '1', F1, 'h' }, CMD_SCREEN_HELP,      "screen-help",
    N_("Help screen") },
  { { '2', F2,  0 }, CMD_SCREEN_PLAY,      "screen-playlist",
    N_("Playlist screen") },
  { { '3', F3,  0 }, CMD_SCREEN_FILE,      "screen-browse",
    N_("Browse screen") },
  { {'u',   0,   0 }, CMD_SCREEN_UPDATE,    "update",
    N_("Update screen") },
#ifdef ENABLE_CLOCK_SCREEN
  { {'4',  F4,   0 }, CMD_SCREEN_CLOCK,    "screen-clock",
    N_("Clock screen") },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
  { {'K',   0,   0 }, CMD_SCREEN_KEYDEF,    "screen-keyedit",
    N_("Key configuration screen") },
#endif

  { { 'q',  0,   0 }, CMD_QUIT,   "quit",
    N_("Quit") },  

  { { -1,  -1,  -1 }, CMD_NONE, NULL, NULL }
};

command_definition_t *
get_command_definitions(void)
{
  return cmds;
}

char *
key2str(int key)
{
  static char buf[32];
  int i;

  buf[0] = 0;
  switch(key)
    {
    case 0:
      return _("Undefined");
    case ' ':
      return _("Space");
    case 13:
      return _("Enter");
    case BS:
      return _("Backspace");
    case DEL:
      return _("Delete");
    case UP: 
      return _("Up");
    case DWN:
      return _("Down");
    case LEFT:
      return _("Left");
    case RGHT:
      return _("Right");
    case HOME:
      return _("Home");
    case END:
      return _("End");
    case PGDN:
      return _("PageDown");
    case PGUP:
      return _("PageUp");
    case TAB: 
      return _("Tab");
    case STAB:
      return _("Shift+Tab");
    case ESC:
      return _("Esc");
    case KEY_IC:
      return _("Insert");
    default:
      for(i=0; i<=63; i++)
	if( key==KEY_F(i) )
	  {
	    snprintf(buf, 32, "F%d", i );
	    return buf;
	  }
      if( !(key & ~037) )
	snprintf(buf, 32, "Ctrl-%c", 'A'+(key & 037)-1 );
      else if( (key & ~037) == 224 )
	snprintf(buf, 32, "Alt-%c", 'A'+(key & 037)-1 );
      else if( key>32 &&  key<256 )
	snprintf(buf, 32, "%c", key);
      else
	snprintf(buf, 32, "0x%03X", key);
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
	return _(cmds[i].description);
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
find_key_command(int key, command_definition_t *cmds)
{
  int i;

  i=0;
  while( cmds && cmds[i].name )
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
get_key_command(int key)
{
  return find_key_command(key, cmds);
}


command_t
get_keyboard_command(void)
{
  int key;

  key = wgetch(stdscr);

  if( key==KEY_RESIZE )
    screen_resize();

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
	    fprintf(stderr, _("Error: Key %s assigned to %s and %s !!!\n"),
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
