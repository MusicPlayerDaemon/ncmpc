/* 
 * (c) 2004 by Kalle Wallin (kaw@linux.se)
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"

#ifdef  ENABLE_KEYDEF_SCREEN
#include "libmpdclient.h"
#include "options.h"
#include "conf.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"

#define STATIC_ITEMS      0
#define STATIC_SUB_ITEMS  1
#define BUFSIZE 256

#define LIST_ITEM_APPLY()   (command_list_length)
#define LIST_ITEM_SAVE()    (LIST_ITEM_APPLY()+1)
#define LIST_LENGTH()       (LIST_ITEM_SAVE()+1)

#define LIST_ITEM_SAVE_LABEL  "===> Apply & Save key bindings  "
#define LIST_ITEM_APPLY_LABEL "===> Apply key bindings "


static list_window_t *lw = NULL;
static int command_list_length = 0;
static command_definition_t *cmds = NULL;

static int subcmd = -1;
static int subcmd_length = 0;
static int subcmd_addpos = 0;

static int
keybindings_changed(void)
{
  command_definition_t *orginal_cmds = get_command_definitions();
  size_t size = command_list_length*sizeof(command_definition_t);
  
  return memcmp(orginal_cmds, cmds, size);
}

static void
apply_keys(void)
{
  if( keybindings_changed() )
    {
      command_definition_t *orginal_cmds = get_command_definitions();
      size_t size = command_list_length*sizeof(command_definition_t);

      memcpy(orginal_cmds, cmds, size);
      screen_status_printf("You have new key bindings!");
    }
  else
    screen_status_printf("Keybindings unchanged.");
}

static int
save_keys(void)
{
  FILE *f;
  char *filename;

  if( check_user_conf_dir() )
    {
      screen_status_printf("Error: Unable to create direcory ~/.ncmpc - %s", 
			   strerror(errno));
      beep();
      return -1;
    }

  filename = get_user_key_binding_filename();

  if( (f=fopen(filename,"w")) == NULL )
    {
      screen_status_printf("Error: %s - %s", filename, strerror(errno));
      beep();
      g_free(filename);
      return -1;
    }
  if( write_key_bindings(f) )
    screen_status_printf("Error: %s - %s", filename, strerror(errno));
  else
    screen_status_printf("Wrote %s", filename);
  
  g_free(filename);
  return fclose(f);
}

static void
check_subcmd_length(void)
{
  subcmd_length = 0;
  while( subcmd_length<MAX_COMMAND_KEYS && cmds[subcmd].keys[subcmd_length]>0 )
   subcmd_length ++;

  if( subcmd_length<MAX_COMMAND_KEYS )
    {
      subcmd_addpos = subcmd_length;
      subcmd_length++;
    }
  else
    subcmd_addpos = 0;
  subcmd_length += STATIC_SUB_ITEMS;
}

static void
delete_key(int cmd_index, int key_index)
{
  int i = key_index+1;

  screen_status_printf("Delete...");
  while( i<MAX_COMMAND_KEYS && cmds[cmd_index].keys[i] )
    cmds[cmd_index].keys[key_index++] = cmds[cmd_index].keys[i++];
  cmds[cmd_index].keys[key_index] = 0;

  check_subcmd_length();
  lw->clear = 1;
  lw->repaint = 1;
}

static void
assign_new_key(WINDOW *w, int cmd_index, int key_index)
{
  int key;
  char buf[BUFSIZE];
  command_t cmd;

  snprintf(buf, BUFSIZE, "Enter new key for %s: ", cmds[cmd_index].name);
  key = screen_getch(w, buf);
  if( key==KEY_RESIZE )
    screen_resize();
  if( key==ERR )
    {
      screen_status_printf("Aborted!");
      return;
    }
  cmd = find_key_command(key, cmds);
  if( cmd!=CMD_NONE && cmd!= cmds[cmd_index].command )
    {
      screen_status_printf("Error: key %s is already used for %s", 
			   key2str(key),
			   get_key_command_name(cmd));
      beep();
      return;
    }
  cmds[cmd_index].keys[key_index] = key;
  screen_status_printf("Assigned %s to %s", key2str(key),cmds[cmd_index].name);
  check_subcmd_length();
  lw->repaint = 1;
}

static char *
list_callback(int index, int *highlight, void *data)
{
  static char buf[BUFSIZE];

  if( subcmd <0 )
    {
      if( index<command_list_length )
	return cmds[index].name;
      else if( index==LIST_ITEM_APPLY() )
	return LIST_ITEM_APPLY_LABEL;
      else if( index==LIST_ITEM_SAVE() )
	return LIST_ITEM_SAVE_LABEL;
    }
  else
  {
    if( index== 0 )
      return "[..]";
    index--;
    if( index<MAX_COMMAND_KEYS && cmds[subcmd].keys[index]>0 )
      {
	snprintf(buf, 
		 BUFSIZE, "%d. %-20s   (%d) ", 
		 index+1, 
		 key2str(cmds[subcmd].keys[index]),
		 cmds[subcmd].keys[index]);
	return buf;
      } 
    else if ( index==subcmd_addpos )
      {
	snprintf(buf, BUFSIZE, "%d. Add new key ", index+1 );
	return buf;
      }
  }
  
  return NULL;
}

static void 
keydef_init(WINDOW *w, int cols, int rows)
{
  lw = list_window_init(w, cols, rows);
}

static void
keydef_resize(int cols, int rows)
{
  lw->cols = cols;
  lw->rows = rows;
}

static void 
keydef_exit(void)
{
  list_window_free(lw);
  if( cmds )
    g_free(cmds);
  cmds = NULL;
  lw = NULL;
}

static void 
keydef_open(screen_t *screen, mpd_client_t *c)
{
  if( cmds == NULL )
    {
      command_definition_t *current_cmds = get_command_definitions();
      size_t cmds_size;

      command_list_length = 0;
      while( current_cmds[command_list_length].name )
	command_list_length++;

      cmds_size = (command_list_length+1)*sizeof(command_definition_t);
      cmds = g_malloc0(cmds_size);
      memcpy(cmds, current_cmds, cmds_size);
      command_list_length += STATIC_ITEMS;
      screen_status_printf("Welcome to the key editor!");
    }

  subcmd = -1;
  list_window_check_selected(lw, LIST_LENGTH());  
}

static void 
keydef_close(void)
{
  if( cmds && !keybindings_changed() )
    {
      g_free(cmds);
      cmds = NULL;
    }
  else
    screen_status_printf("Note: Did you forget to \'Apply\' your changes?");
}

static char *
keydef_title(void)
{
  static char buf[BUFSIZE];

  if( subcmd<0 )
    return (TOP_HEADER_PREFIX "Edit key bindings");
  
  snprintf(buf, BUFSIZE, 
	   TOP_HEADER_PREFIX "Edit keys for %s", 
	   cmds[subcmd].name);
  return buf;
}

static void 
keydef_paint(screen_t *screen, mpd_client_t *c)
{
  lw->clear = 1;
  list_window_paint(lw, list_callback, NULL);
  wrefresh(lw->w);
}

static void 
keydef_update(screen_t *screen, mpd_client_t *c)
{  
  if( lw->repaint )
    {
      list_window_paint(lw, list_callback, NULL);
      wrefresh(lw->w);
      lw->repaint = 0;
    }
}

static int 
keydef_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
  int length = LIST_LENGTH();

  if( subcmd>=0 )
    length = subcmd_length;

  switch(cmd)
    {
    case CMD_PLAY:
      if( subcmd<0 )
	{
	  if( lw->selected == LIST_ITEM_APPLY() )
	    apply_keys();
	  else if( lw->selected == LIST_ITEM_SAVE() )
	    {
	      apply_keys();
	      save_keys();
	    }
	  else
	    {
	      subcmd = lw->selected;
	      lw->selected=0;
	      check_subcmd_length();
	    }
	}
      else
	{
	  if( lw->selected == 0 ) /* up */
	    {
	      lw->selected = subcmd;
	      subcmd = -1;
	    }
	  else
	    assign_new_key(screen->status_window.w, 
			   subcmd,
			   lw->selected-STATIC_SUB_ITEMS);
	}
      lw->repaint = 1;
      lw->clear = 1;
      return 1;
    case CMD_DELETE:
      if( subcmd>=0 && lw->selected-STATIC_SUB_ITEMS>=0 )
	delete_key(subcmd, lw->selected-STATIC_SUB_ITEMS);
      return 1;
      break;
    case CMD_LIST_FIND:
    case CMD_LIST_RFIND:
    case CMD_LIST_FIND_NEXT:
    case CMD_LIST_RFIND_NEXT:
      return screen_find(screen, c, 
			 lw,  length,
			 cmd, list_callback);

    default:
      break;
    }

  return list_window_cmd(lw, length, cmd);
}

static list_window_t *
keydef_lw(void)
{
  return lw;
}

screen_functions_t *
get_screen_keydef(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = keydef_init;
  functions.exit   = keydef_exit;
  functions.open   = keydef_open;
  functions.close  = keydef_close;
  functions.resize = keydef_resize;
  functions.paint  = keydef_paint;
  functions.update = keydef_update;
  functions.cmd    = keydef_cmd;
  functions.get_lw = keydef_lw;
  functions.get_title = keydef_title;

  return &functions;
}


#endif
