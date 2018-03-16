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

#include "screen_keydef.hxx"
#include "screen_interface.hxx"
#include "ListPage.hxx"
#include "ProxyPage.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "conf.hxx"
#include "screen_utils.hxx"
#include "options.hxx"
#include "Compiler.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <glib.h>

class CommandKeysPage final : public ListPage {
	ScreenManager &screen;

	command_definition_t *cmds;

	/**
	 * The command being edited, represented by a array subscript
	 * to @cmds, or -1, if no command is being edited
	 */
	int subcmd = -1;

	/** The number of keys assigned to the current command */
	unsigned subcmd_n_keys = 0;

public:
	CommandKeysPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size), screen(_screen) {}

	void SetCommand(command_definition_t *_cmds, unsigned _cmd) {
		cmds = _cmds;
		subcmd = _cmd;
		lw.Reset();
		check_subcmd_length();
	}

private:
	/** The position of the up ("[..]") item */
	static constexpr unsigned subcmd_item_up() {
		return 0;
	}

	/** The position of the "add a key" item */
	gcc_pure
	unsigned subcmd_item_add() const {
		return subcmd_n_keys + 1;
	}

	/** The number of items in the list_window, if there's a command being edited */
	gcc_pure
	unsigned subcmd_length() const {
		return subcmd_item_add() + 1;
	}

	/** Check whether a given item is a key */
	gcc_pure
	bool subcmd_item_is_key(unsigned i) const {
		return (i > subcmd_item_up() && i < subcmd_item_add());
	}

	/**
	 * Convert an item id (as in lw.selected) into a "key id", which is an array
	 * subscript to cmds[subcmd].keys.
	 */
	static constexpr unsigned subcmd_item_to_key_id(unsigned i) {
		return i - 1;
	}

	/* TODO: rename to check_n_keys / subcmd_count_keys? */
	void check_subcmd_length();

	/**
	 * Delete a key from a given command's definition.
	 *
	 * @param cmd_index the command
	 * @param key_index the key (see below)
	 */
	void DeleteKey(int cmd_index, int key_index);

	/**
	 * Assigns a new key to a key slot.
	 */
	void OverwriteKey(int cmd_index, int key_index);

	/**
	 * Assign a new key to a new slot.
	 */
	void AddKey(int cmd_index);

	const char *ListCallback(unsigned idx) const;

	static const char *ListCallback(unsigned idx, void *data) {
		const auto &p = *(const CommandKeysPage *)data;
		return p.ListCallback(idx);
	}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

/* TODO: rename to check_n_keys / subcmd_count_keys? */
void
CommandKeysPage::check_subcmd_length()
{
	unsigned i;

	/* this loops counts the continous valid keys at the start of the the keys
	   array, so make sure you don't have gaps */
	for (i = 0; i < MAX_COMMAND_KEYS; i++)
		if (cmds[subcmd].keys[i] == 0)
			break;
	subcmd_n_keys = i;

	lw.SetLength(subcmd_length());
}

void
CommandKeysPage::DeleteKey(int cmd_index, int key_index)
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
	SetDirty();

	/* update key conflict flags */
	check_key_bindings(cmds, nullptr, 0);
}

void
CommandKeysPage::OverwriteKey(int cmd_index, int key_index)
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
	SetDirty();

	/* update key conflict flags */
	check_key_bindings(cmds, nullptr, 0);
}

void
CommandKeysPage::AddKey(int cmd_index)
{
	if (subcmd_n_keys < MAX_COMMAND_KEYS)
		OverwriteKey(cmd_index, subcmd_n_keys);
}

const char *
CommandKeysPage::ListCallback(unsigned idx) const
{
	static char buf[256];

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

void
CommandKeysPage::OnOpen(gcc_unused struct mpdclient &c)
{
	// TODO
}

const char *
CommandKeysPage::GetTitle(char *str, size_t size) const
{
	g_snprintf(str, size, _("Edit keys for %s"), cmds[subcmd].name);
	return str;
}

void
CommandKeysPage::Paint() const
{
	lw.Paint(ListCallback, const_cast<CommandKeysPage *>(this));
}

bool
CommandKeysPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (cmd == CMD_LIST_RANGE_SELECT)
		return false;

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case CMD_PLAY:
		if (lw.selected == subcmd_item_up()) {
			screen.OnCommand(c, CMD_GO_PARENT_DIRECTORY);
		} else if (lw.selected == subcmd_item_add()) {
			AddKey(subcmd);
		} else {
			/* just to be sure ;-) */
			assert(subcmd_item_is_key(lw.selected));
			OverwriteKey(subcmd, subcmd_item_to_key_id(lw.selected));
		}
		return true;
	case CMD_DELETE:
		if (subcmd_item_is_key(lw.selected))
			DeleteKey(subcmd, subcmd_item_to_key_id(lw.selected));

		return true;
	case CMD_ADD:
		AddKey(subcmd);
		return true;
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(screen, &lw, cmd, ListCallback, this);
		SetDirty();
		return true;

	default:
		return false;
	}

	/* unreachable */
	assert(0);
	return false;
}

class CommandListPage final : public ListPage {
	ScreenManager &screen;

	command_definition_t *cmds = nullptr;

	/** the number of commands */
	unsigned command_n_commands = 0;

public:
	CommandListPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size), screen(_screen) {}

	~CommandListPage() override {
		delete[] cmds;
	}

	command_definition_t *GetCommands() {
		return cmds;
	}

	int GetSelectedCommand() const {
		return lw.selected < command_n_commands
			? (int)lw.selected
			: -1;
	}

private:
	/**
	 * the position of the "apply" item. It's the same as command_n_commands,
	 * because array subscripts start at 0, while numbers of items start at 1.
	 */
	gcc_pure
	unsigned command_item_apply() const {
		return command_n_commands;
	}

	/** the position of the "apply and save" item */
	gcc_pure
	unsigned command_item_save() const {
		return command_item_apply() + 1;
	}

	/** the number of items in the "command" view */
	gcc_pure
	unsigned command_length() const {
		return command_item_save() + 1;
	}

	/** The position of the up ("[..]") item */
	static constexpr unsigned subcmd_item_up() {
		return 0;
	}

public:
	bool IsModified() const;

	void Apply();
	void Save();

private:
	const char *ListCallback(unsigned idx) const;

	static const char *ListCallback(unsigned idx, void *data) {
		const auto &p = *(const CommandListPage *)data;
		return p.ListCallback(idx);
	}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

bool
CommandListPage::IsModified() const
{
	command_definition_t *orginal_cmds = get_command_definitions();
	size_t size = command_n_commands * sizeof(command_definition_t);

	return memcmp(orginal_cmds, cmds, size) != 0;
}

void
CommandListPage::Apply()
{
	if (IsModified()) {
		command_definition_t *orginal_cmds = get_command_definitions();
		size_t size = command_n_commands * sizeof(command_definition_t);

		memcpy(orginal_cmds, cmds, size);
		screen_status_printf(_("You have new key bindings"));
	} else
		screen_status_printf(_("Keybindings unchanged."));
}

void
CommandListPage::Save()
{
	char *allocated = nullptr;
	const char *filename = options.key_file;
	if (filename == nullptr) {
		if (!check_user_conf_dir()) {
			screen_status_printf(_("Error: Unable to create directory ~/.ncmpc - %s"),
					     strerror(errno));
			screen_bell();
			return;
		}

		filename = allocated = build_user_key_binding_filename();
	}

	FILE *f = fopen(filename, "w");
	if (f == nullptr) {
		screen_status_printf(_("Error: %s - %s"), filename, strerror(errno));
		screen_bell();
		g_free(allocated);
		return;
	}

	if (write_key_bindings(f, KEYDEF_WRITE_HEADER))
		screen_status_printf(_("Wrote %s"), filename);
	else
		screen_status_printf(_("Error: %s - %s"), filename, strerror(errno));

	g_free(allocated);
	fclose(f);
}

const char *
CommandListPage::ListCallback(unsigned idx) const
{
	static char buf[256];

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
}

void
CommandListPage::OnOpen(gcc_unused struct mpdclient &c)
{
	if (cmds == nullptr) {
		command_definition_t *current_cmds = get_command_definitions();
		command_n_commands = 0;
		while (current_cmds[command_n_commands].name)
			command_n_commands++;

		/* +1 for the terminator element */
		size_t cmds_size = (command_n_commands + 1)
			* sizeof(command_definition_t);
		cmds = new command_definition_t[command_n_commands + 1];
		memcpy(cmds, current_cmds, cmds_size);
	}

	lw.SetLength(command_length());
}

const char *
CommandListPage::GetTitle(char *, size_t) const
{
	return _("Edit key bindings");
}

void
CommandListPage::Paint() const
{
	lw.Paint(ListCallback, const_cast<CommandListPage *>(this));
}

bool
CommandListPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (cmd == CMD_LIST_RANGE_SELECT)
		return false;

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case CMD_PLAY:
		if (lw.selected == command_item_apply()) {
			Apply();
			return true;
		} else if (lw.selected == command_item_save()) {
			Apply();
			Save();
			return true;
		}

		break;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(screen, &lw, cmd, ListCallback, this);
		SetDirty();
		return true;

	default:
		break;
	}

	return false;
}

class KeyDefPage final : public ProxyPage {
	ScreenManager &screen;

	CommandListPage command_list_page;
	CommandKeysPage command_keys_page;

public:
	KeyDefPage(ScreenManager &_screen, WINDOW *_w, Size size)
		:ProxyPage(_w), screen(_screen),
		 command_list_page(_screen, _w, size),
		 command_keys_page(_screen, _w, size) {}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void OnClose() override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
};

static Page *
keydef_init(ScreenManager &screen, WINDOW *w, Size size)
{
	return new KeyDefPage(screen, w, size);
}

void
KeyDefPage::OnOpen(struct mpdclient &c)
{
	ProxyPage::OnOpen(c);

	if (GetCurrentPage() == nullptr)
		SetCurrentPage(c, &command_list_page);
}

void
KeyDefPage::OnClose()
{
	if (command_list_page.IsModified())
		screen_status_printf(_("Note: Did you forget to \'Apply\' your changes?"));

	ProxyPage::OnClose();
}

bool
KeyDefPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (ProxyPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case CMD_PLAY:
		if (GetCurrentPage() == &command_list_page) {
			int s = command_list_page.GetSelectedCommand();
			if (s >= 0) {
				command_keys_page.SetCommand(command_list_page.GetCommands(),
							     s);
				SetCurrentPage(c, &command_keys_page);
				return true;
			}
		}

		break;

	case CMD_GO_PARENT_DIRECTORY:
	case CMD_GO_ROOT_DIRECTORY:
		if (GetCurrentPage() != &command_list_page) {
			SetCurrentPage(c, &command_list_page);
			return true;
		}

		break;

	case CMD_SAVE_PLAYLIST:
		command_list_page.Apply();
		command_list_page.Save();
		return true;

	default:
		return false;
	}

	/* unreachable */
	assert(0);
	return false;
}

const struct screen_functions screen_keydef = {
	"keydef",
	keydef_init,
};
