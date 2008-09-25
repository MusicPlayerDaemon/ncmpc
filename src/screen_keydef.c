/*
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

#include "config.h"

#ifndef  DISABLE_KEYDEF_SCREEN
#include "ncmpc.h"
#include "mpdclient.h"
#include "options.h"
#include "conf.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "gcc.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#define STATIC_ITEMS      0
#define STATIC_SUB_ITEMS  1
#define BUFSIZE 256

#define LIST_ITEM_APPLY()   ((unsigned)command_list_length)
#define LIST_ITEM_SAVE()    (LIST_ITEM_APPLY()+1)
#define LIST_LENGTH()       (LIST_ITEM_SAVE()+1)

#define LIST_ITEM_SAVE_LABEL  _("===> Apply & Save key bindings  ")
#define LIST_ITEM_APPLY_LABEL _("===> Apply key bindings ")


static list_window_t *lw = NULL;
static unsigned command_list_length = 0;
static command_definition_t *cmds = NULL;

static int subcmd = -1;
static unsigned subcmd_length = 0;
static unsigned subcmd_addpos = 0;

static int
keybindings_changed(void)
{
	command_definition_t *orginal_cmds = get_command_definitions();
	size_t size = command_list_length * sizeof(command_definition_t);

	return memcmp(orginal_cmds, cmds, size);
}

static void
apply_keys(void)
{
	if (keybindings_changed()) {
		command_definition_t *orginal_cmds = get_command_definitions();
		size_t size = command_list_length * sizeof(command_definition_t);

		memcpy(orginal_cmds, cmds, size);
		screen_status_printf(_("You have new key bindings!"));
	} else
		screen_status_printf(_("Keybindings unchanged."));
}

static int
save_keys(void)
{
	FILE *f;
	char *filename;

	if (check_user_conf_dir()) {
		screen_status_printf(_("Error: Unable to create direcory ~/.ncmpc - %s"),
				     strerror(errno));
		screen_bell();
		return -1;
	}

	filename = get_user_key_binding_filename();

	if ((f = fopen(filename,"w")) == NULL) {
		screen_status_printf(_("Error: %s - %s"), filename, strerror(errno));
		screen_bell();
		g_free(filename);
		return -1;
	}

	if (write_key_bindings(f, KEYDEF_WRITE_HEADER))
		screen_status_printf(_("Error: %s - %s"), filename, strerror(errno));
	else
		screen_status_printf(_("Wrote %s"), filename);

	g_free(filename);
	return fclose(f);
}

static void
check_subcmd_length(void)
{
	subcmd_length = 0;
	while (subcmd_length < MAX_COMMAND_KEYS &&
	       cmds[subcmd].keys[subcmd_length] > 0)
		++subcmd_length;

	if (subcmd_length < MAX_COMMAND_KEYS) {
		subcmd_addpos = subcmd_length;
		subcmd_length++;
	} else
		subcmd_addpos = 0;
	subcmd_length += STATIC_SUB_ITEMS;
}

static void
delete_key(int cmd_index, int key_index)
{
	int i = key_index+1;

	screen_status_printf(_("Deleted"));
	while (i < MAX_COMMAND_KEYS && cmds[cmd_index].keys[i])
		cmds[cmd_index].keys[key_index++] = cmds[cmd_index].keys[i++];
	cmds[cmd_index].keys[key_index] = 0;
	cmds[cmd_index].flags |= COMMAND_KEY_MODIFIED;
	check_subcmd_length();
	lw->repaint = 1;
	/* update key conflict flags */
	check_key_bindings(cmds, NULL, 0);
}

static void
assign_new_key(WINDOW *w, int cmd_index, int key_index)
{
	int key;
	char *buf;
	command_t cmd;

	buf = g_strdup_printf(_("Enter new key for %s: "), cmds[cmd_index].name);
	key = screen_getch(w, buf);
	g_free(buf);

	if (key == KEY_RESIZE)
		screen_resize();

	if (key==ERR) {
		screen_status_printf(_("Aborted!"));
		return;
	}

	cmd = find_key_command(key, cmds);
	if (cmd != CMD_NONE && cmd != cmds[cmd_index].command) {
		screen_status_printf(_("Error: key %s is already used for %s"),
				     key2str(key),
				     get_key_command_name(cmd));
		screen_bell();
		return;
	}

	cmds[cmd_index].keys[key_index] = key;
	cmds[cmd_index].flags |= COMMAND_KEY_MODIFIED;

	screen_status_printf(_("Assigned %s to %s"),
			     key2str(key),cmds[cmd_index].name);
	check_subcmd_length();
	lw->repaint = 1;
	/* update key conflict flags */
	check_key_bindings(cmds, NULL, 0);
}

static const char *
list_callback(unsigned idx, int *highlight, mpd_unused void *data)
{
	static char buf[BUFSIZE];

	if (subcmd < 0) {
		if (idx < (unsigned)command_list_length) {
			if (cmds[idx].flags & COMMAND_KEY_CONFLICT)
				*highlight = 1;
			return cmds[idx].name;
		} else if (idx == LIST_ITEM_APPLY())
			return LIST_ITEM_APPLY_LABEL;
		else if (idx == LIST_ITEM_SAVE())
			return LIST_ITEM_SAVE_LABEL;
	} else {
		if (idx == 0)
			return "[..]";
		idx--;
		if (idx < MAX_COMMAND_KEYS && cmds[subcmd].keys[idx] > 0) {
			g_snprintf(buf,
				   BUFSIZE, "%d. %-20s   (%d) ",
				   idx + 1,
				   key2str(cmds[subcmd].keys[idx]),
				   cmds[subcmd].keys[idx]);
			return buf;
		} else if (idx == subcmd_addpos) {
			g_snprintf(buf, BUFSIZE, _("%d. Add new key "), idx + 1);
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
	if (cmds)
		g_free(cmds);
	cmds = NULL;
	lw = NULL;
}

static void
keydef_open(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	if (cmds == NULL) {
		command_definition_t *current_cmds = get_command_definitions();
		size_t cmds_size;

		command_list_length = 0;
		while (current_cmds[command_list_length].name)
			command_list_length++;

		cmds_size = (command_list_length+1) * sizeof(command_definition_t);
		cmds = g_malloc0(cmds_size);
		memcpy(cmds, current_cmds, cmds_size);
		command_list_length += STATIC_ITEMS;
		screen_status_printf(_("Welcome to the key editor!"));
	}

	subcmd = -1;
	list_window_check_selected(lw, LIST_LENGTH());
}

static void
keydef_close(void)
{
	if (cmds && !keybindings_changed()) {
		g_free(cmds);
		cmds = NULL;
	} else
		screen_status_printf(_("Note: Did you forget to \'Apply\' your changes?"));
}

static const char *
keydef_title(char *str, size_t size)
{
	if (subcmd < 0)
		return _("Edit key bindings");

	g_snprintf(str, size, _("Edit keys for %s"), cmds[subcmd].name);
	return str;
}

static void
keydef_paint(mpd_unused mpdclient_t *c)
{
	list_window_paint(lw, list_callback, NULL);
}

static void
keydef_update(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	if (lw->repaint) {
		list_window_paint(lw, list_callback, NULL);
		lw->repaint = 0;
	}
}

static int
keydef_cmd(screen_t *screen, mpd_unused mpdclient_t *c, command_t cmd)
{
	int length = LIST_LENGTH();

	if (subcmd >= 0)
		length = subcmd_length;

	switch(cmd) {
	case CMD_PLAY:
		if (subcmd < 0) {
			if (lw->selected == LIST_ITEM_APPLY())
				apply_keys();
			else if (lw->selected == LIST_ITEM_SAVE()) {
				apply_keys();
				save_keys();
			} else {
				subcmd = lw->selected;
				lw->selected=0;
				check_subcmd_length();
			}
		} else {
			if (lw->selected == 0) { /* up */
				lw->selected = subcmd;
				subcmd = -1;
			} else
				assign_new_key(screen->status_window.w,
					       subcmd,
					       lw->selected - STATIC_SUB_ITEMS);
		}
		lw->repaint = 1;
		return 1;
	case CMD_DELETE:
		if (subcmd >= 0 && lw->selected >= STATIC_SUB_ITEMS)
			delete_key(subcmd, lw->selected - STATIC_SUB_ITEMS);
		return 1;
		break;
	case CMD_SAVE_PLAYLIST:
		apply_keys();
		save_keys();
		break;
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		return screen_find(screen,
				   lw,  length,
				   cmd, list_callback, NULL);

	default:
		break;
	}

	return list_window_cmd(lw, length, cmd);
}

const struct screen_functions screen_keydef = {
	.init = keydef_init,
	.exit = keydef_exit,
	.open = keydef_open,
	.close = keydef_close,
	.resize = keydef_resize,
	.paint = keydef_paint,
	.update = keydef_update,
	.cmd = keydef_cmd,
	.get_title = keydef_title,
};

#endif
