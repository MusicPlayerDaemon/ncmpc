/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#include "screen_keydef.h"
#include "screen_interface.h"
#include "screen_status.h"
#include "screen_find.h"
#include "i18n.h"
#include "conf.h"
#include "screen.h"
#include "screen_utils.h"
#include "Compiler.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <glib.h>

static struct list_window *lw;

static command_definition_t *cmds = NULL;

/** the number of commands */
static unsigned command_n_commands = 0;

/**
 * the position of the "apply" item. It's the same as command_n_commands,
 * because array subscripts start at 0, while numbers of items start at 1.
 */
gcc_pure
static inline unsigned
command_item_apply(void)
{
	return command_n_commands;
}

/** the position of the "apply and save" item */
gcc_pure
static inline unsigned
command_item_save(void)
{
	return command_item_apply() + 1;
}

/** the number of items in the "command" view */
gcc_pure
static inline unsigned
command_length(void)
{
	return command_item_save() + 1;
}


/**
 * The command being edited, represented by a array subscript to @cmds, or -1,
 * if no command is being edited
 */
static int subcmd = -1;

/** The number of keys assigned to the current command */
static unsigned subcmd_n_keys = 0;

/** The position of the up ("[..]") item */
gcc_const
static inline unsigned
subcmd_item_up(void)
{
	return 0;
}

/** The position of the "add a key" item */
gcc_pure
static inline unsigned
subcmd_item_add(void)
{
	return subcmd_n_keys + 1;
}

/** The number of items in the list_window, if there's a command being edited */
gcc_pure
static inline unsigned
subcmd_length(void)
{
	return subcmd_item_add() + 1;
}

/** Check whether a given item is a key */
gcc_pure
static inline bool
subcmd_item_is_key(unsigned i)
{
	return (i > subcmd_item_up() && i < subcmd_item_add());
}

/**
 * Convert an item id (as in lw->selected) into a "key id", which is an array
 * subscript to cmds[subcmd].keys.
 */
gcc_const
static inline unsigned
subcmd_item_to_key_id(unsigned i)
{
	return i - 1;
}


static int
keybindings_changed(void)
{
	command_definition_t *orginal_cmds = get_command_definitions();
	size_t size = command_n_commands * sizeof(command_definition_t);

	return memcmp(orginal_cmds, cmds, size);
}

static void
apply_keys(void)
{
	if (keybindings_changed()) {
		command_definition_t *orginal_cmds = get_command_definitions();
		size_t size = command_n_commands * sizeof(command_definition_t);

		memcpy(orginal_cmds, cmds, size);
		screen_status_printf(_("You have new key bindings"));
	} else
		screen_status_printf(_("Keybindings unchanged."));
}

static int
save_keys(void)
{
	if (!check_user_conf_dir()) {
		screen_status_printf(_("Error: Unable to create directory ~/.ncmpc - %s"),
				     strerror(errno));
		screen_bell();
		return -1;
	}

	char *filename = build_user_key_binding_filename();
	FILE *f = fopen(filename, "w");
	if (f == NULL) {
		screen_status_printf(_("Error: %s - %s"), filename, strerror(errno));
		screen_bell();
		g_free(filename);
		return -1;
	}

	if (write_key_bindings(f, KEYDEF_WRITE_HEADER))
		screen_status_printf(_("Wrote %s"), filename);
	else
		screen_status_printf(_("Error: %s - %s"), filename, strerror(errno));

	g_free(filename);
	return fclose(f);
}

/* TODO: rename to check_n_keys / subcmd_count_keys? */
static void
check_subcmd_length(void)
{
	unsigned i;

	/* this loops counts the continous valid keys at the start of the the keys
	   array, so make sure you don't have gaps */
	for (i = 0; i < MAX_COMMAND_KEYS; i++)
		if (cmds[subcmd].keys[i] == 0)
			break;
	subcmd_n_keys = i;

	list_window_set_length(lw, subcmd_length());
}

static void
keydef_paint(void);

static void
keydef_repaint(void)
{
	keydef_paint();
	wrefresh(lw->w);
}

/** lw->start the last time switch_to_subcmd_mode() was called */
static unsigned saved_start = 0;

static void
switch_to_subcmd_mode(int cmd)
{
	assert(subcmd == -1);

	saved_start = lw->start;

	subcmd = cmd;
	list_window_reset(lw);
	check_subcmd_length();

	keydef_repaint();
}

static void
switch_to_command_mode(void)
{
	assert(subcmd != -1);

	list_window_set_length(lw, command_length());
	list_window_set_cursor(lw, subcmd);
	subcmd = -1;

	lw->start = saved_start;

	keydef_repaint();
}

/**
 * Delete a key from a given command's definition
 * @param cmd_index the command
 * @param key_index the key (see below)
 */
static void
delete_key(int cmd_index, int key_index)
{
	/* shift the keys to close the gap that appeared */
	int i = key_index+1;
	while (i < MAX_COMMAND_KEYS && cmds[cmd_index].keys[i])
		cmds[cmd_index].keys[key_index++] = cmds[cmd_index].keys[i++];

	/* As key_index now holds the index of the last key slot that contained
	   a key, we use it to empty this slot, because this key has been copied
	   to the previous slot in the loop above */
	cmds[cmd_index].keys[key_index] = 0;

	cmds[cmd_index].flags |= COMMAND_KEY_MODIFIED;
	check_subcmd_length();

	screen_status_printf(_("Deleted"));

	/* repaint */
	keydef_repaint();

	/* update key conflict flags */
	check_key_bindings(cmds, NULL, 0);
}

/* assigns a new key to a key slot */
static void
overwrite_key(int cmd_index, int key_index)
{
	assert(key_index < MAX_COMMAND_KEYS);

	char *buf = g_strdup_printf(_("Enter new key for %s: "),
				    cmds[cmd_index].name);
	const int key = screen_getch(buf);
	g_free(buf);

	if (key == ERR) {
		screen_status_printf(_("Aborted"));
		return;
	}

	if (key == '\0') {
		screen_status_printf(_("Ctrl-Space can't be used"));
		return;
	}

	const command_t cmd = find_key_command(key, cmds);
	if (cmd != CMD_NONE) {
		screen_status_printf(_("Error: key %s is already used for %s"),
				     key2str(key), get_key_command_name(cmd));
		screen_bell();
		return;
	}

	cmds[cmd_index].keys[key_index] = key;
	cmds[cmd_index].flags |= COMMAND_KEY_MODIFIED;

	screen_status_printf(_("Assigned %s to %s"),
			     key2str(key),cmds[cmd_index].name);
	check_subcmd_length();

	/* repaint */
	keydef_repaint();

	/* update key conflict flags */
	check_key_bindings(cmds, NULL, 0);
}

/* assign a new key to a new slot */
static void
add_key(int cmd_index)
{
	if (subcmd_n_keys < MAX_COMMAND_KEYS)
		overwrite_key(cmd_index, subcmd_n_keys);
}

static const char *
list_callback(unsigned idx, gcc_unused void *data)
{
	static char buf[256];

	if (subcmd == -1) {
		if (idx == command_item_apply())
			return _("===> Apply key bindings ");
		if (idx == command_item_save())
			return _("===> Apply & Save key bindings  ");

		assert(idx < (unsigned) command_n_commands);

		/*
		 * Format the lines in two aligned columnes for the key name and
		 * the description, like this:
		 *
		 *	this-command - do this
		 *	that-one     - do that
		 */
		size_t len = strlen(cmds[idx].name);
		strncpy(buf, cmds[idx].name, sizeof(buf));

		if (len < get_cmds_max_name_width(cmds))
			memset(buf + len, ' ', get_cmds_max_name_width(cmds) - len);

		g_snprintf(buf + get_cmds_max_name_width(cmds),
			   sizeof(buf) - get_cmds_max_name_width(cmds),
			   " - %s", _(cmds[idx].description));

		return buf;
	} else {
		if (idx == subcmd_item_up())
			return "[..]";

		if (idx == subcmd_item_add()) {
			g_snprintf(buf, sizeof(buf), "%d. %s",
				   idx, _("Add new key"));
			return buf;
		}

		assert(subcmd_item_is_key(idx));

		g_snprintf(buf, sizeof(buf),
			   "%d. %-20s   (%d) ", idx,
			   key2str(cmds[subcmd].keys[subcmd_item_to_key_id(idx)]),
			   cmds[subcmd].keys[subcmd_item_to_key_id(idx)]);
		return buf;
	}
}

static void
keydef_init(WINDOW *w, int cols, int rows)
{
	lw = list_window_init(w, cols, rows);
}

static void
keydef_resize(int cols, int rows)
{
	list_window_resize(lw, cols, rows);
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
keydef_open(gcc_unused struct mpdclient *c)
{
	if (cmds == NULL) {
		command_definition_t *current_cmds = get_command_definitions();
		command_n_commands = 0;
		while (current_cmds[command_n_commands].name)
			command_n_commands++;

		/* +1 for the terminator element */
		size_t cmds_size = (command_n_commands + 1)
			* sizeof(command_definition_t);
		cmds = g_malloc0(cmds_size);
		memcpy(cmds, current_cmds, cmds_size);
	}

	subcmd = -1;
	list_window_set_length(lw, command_length());
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
	if (subcmd == -1)
		return _("Edit key bindings");

	g_snprintf(str, size, _("Edit keys for %s"), cmds[subcmd].name);
	return str;
}

static void
keydef_paint(void)
{
	list_window_paint(lw, list_callback, NULL);
}

static bool
keydef_cmd(gcc_unused struct mpdclient *c, command_t cmd)
{
	if (cmd == CMD_LIST_RANGE_SELECT)
		return false;

	if (list_window_cmd(lw, cmd)) {
		keydef_repaint();
		return true;
	}

	switch(cmd) {
	case CMD_PLAY:
		if (subcmd == -1) {
			if (lw->selected == command_item_apply()) {
				apply_keys();
			} else if (lw->selected == command_item_save()) {
				apply_keys();
				save_keys();
			} else {
				switch_to_subcmd_mode(lw->selected);
			}
		} else {
			if (lw->selected == subcmd_item_up()) {
				switch_to_command_mode();
			} else if (lw->selected == subcmd_item_add()) {
				add_key(subcmd);
			} else {
				/* just to be sure ;-) */
				assert(subcmd_item_is_key(lw->selected));
				overwrite_key(subcmd, subcmd_item_to_key_id(lw->selected));
			}
		}
		return true;
	case CMD_GO_PARENT_DIRECTORY:
	case CMD_GO_ROOT_DIRECTORY:
		if (subcmd != -1)
			switch_to_command_mode();
		return true;
	case CMD_DELETE:
		if (subcmd != -1 && subcmd_item_is_key(lw->selected))
			delete_key(subcmd, subcmd_item_to_key_id(lw->selected));

		return true;
	case CMD_ADD:
		if (subcmd != -1)
			add_key(subcmd);
		return true;
	case CMD_SAVE_PLAYLIST:
		apply_keys();
		save_keys();
		return true;
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(lw, cmd, list_callback, NULL);
		keydef_repaint();
		return true;

	default:
		return false;
	}

	/* unreachable */
	assert(0);
	return false;
}

const struct screen_functions screen_keydef = {
	.init = keydef_init,
	.exit = keydef_exit,
	.open = keydef_open,
	.close = keydef_close,
	.resize = keydef_resize,
	.paint = keydef_paint,
	.cmd = keydef_cmd,
	.get_title = keydef_title,
};
