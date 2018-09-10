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
#include "PageMeta.hxx"
#include "ListPage.hxx"
#include "ListText.hxx"
#include "TextListRenderer.hxx"
#include "ProxyPage.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "screen.hxx"
#include "KeyName.hxx"
#include "i18n.h"
#include "conf.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "screen_utils.hxx"
#include "options.hxx"
#include "Compiler.h"

#include <algorithm>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <glib.h>

class CommandKeysPage final : public ListPage, ListText {
	ScreenManager &screen;

	const KeyBindings *bindings;
	KeyBinding *binding;

	/**
	 * The command being edited, represented by a array subscript
	 * to #bindings, or -1, if no command is being edited
	 */
	int subcmd = -1;

	/** The number of keys assigned to the current command */
	unsigned subcmd_n_keys = 0;

public:
	CommandKeysPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size), screen(_screen) {}

	void SetCommand(KeyBindings *_bindings, unsigned _cmd) {
		bindings = _bindings;
		binding = &_bindings->key_bindings[_cmd];
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
	 * @param key_index the key (see below)
	 */
	void DeleteKey(int key_index);

	/**
	 * Assigns a new key to a key slot.
	 */
	void OverwriteKey(int key_index);

	/**
	 * Assign a new key to a new slot.
	 */
	void AddKey();

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const override;

private:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const override;
};

/* TODO: rename to check_n_keys / subcmd_count_keys? */
void
CommandKeysPage::check_subcmd_length()
{
	subcmd_n_keys = binding->GetKeyCount();

	lw.SetLength(subcmd_length());
}

void
CommandKeysPage::DeleteKey(int key_index)
{
	/* shift the keys to close the gap that appeared */
	int i = key_index+1;
	while (i < MAX_COMMAND_KEYS && binding->keys[i])
		binding->keys[key_index++] = binding->keys[i++];

	/* As key_index now holds the index of the last key slot that contained
	   a key, we use it to empty this slot, because this key has been copied
	   to the previous slot in the loop above */
	binding->keys[key_index] = 0;

	binding->modified = true;
	check_subcmd_length();

	screen_status_message(_("Deleted"));

	/* repaint */
	SetDirty();

	/* update key conflict flags */
	bindings->Check(nullptr, 0);
}

void
CommandKeysPage::OverwriteKey(int key_index)
{
	assert(key_index < MAX_COMMAND_KEYS);

	char prompt[256];
	snprintf(prompt, sizeof(prompt),
		 _("Enter new key for %s: "),
		 get_key_command_name(Command(subcmd)));
	const int key = screen_getch(prompt);

	if (key == ERR) {
		screen_status_message(_("Aborted"));
		return;
	}

	if (key == '\0') {
		screen_status_message(_("Ctrl-Space can't be used"));
		return;
	}

	const Command cmd = bindings->FindKey(key);
	if (cmd != Command::NONE) {
		screen_status_printf(_("Error: key %s is already used for %s"),
				     key2str(key), get_key_command_name(cmd));
		screen_bell();
		return;
	}

	binding->keys[key_index] = key;
	binding->modified = true;

	screen_status_printf(_("Assigned %s to %s"),
			     key2str(key),
			     get_key_command_name(Command(subcmd)));
	check_subcmd_length();

	/* repaint */
	SetDirty();

	/* update key conflict flags */
	bindings->Check(nullptr, 0);
}

void
CommandKeysPage::AddKey()
{
	if (subcmd_n_keys < MAX_COMMAND_KEYS)
		OverwriteKey(subcmd_n_keys);
}

const char *
CommandKeysPage::GetListItemText(char *buffer, size_t size,
				 unsigned idx) const
{
	if (idx == subcmd_item_up())
		return "[..]";

	if (idx == subcmd_item_add()) {
		snprintf(buffer, size, "%d. %s", idx, _("Add new key"));
		return buffer;
	}

	assert(subcmd_item_is_key(idx));

	snprintf(buffer, size,
		 "%d. %-20s   (%d) ", idx,
		 key2str(binding->keys[subcmd_item_to_key_id(idx)]),
		 binding->keys[subcmd_item_to_key_id(idx)]);
	return buffer;
}

void
CommandKeysPage::OnOpen(gcc_unused struct mpdclient &c)
{
	// TODO
}

const char *
CommandKeysPage::GetTitle(char *str, size_t size) const
{
	snprintf(str, size, _("Edit keys for %s"),
		 get_key_command_name(Command(subcmd)));
	return str;
}

void
CommandKeysPage::Paint() const
{
	lw.Paint(TextListRenderer(*this));
}

bool
CommandKeysPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (cmd == Command::LIST_RANGE_SELECT)
		return false;

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case Command::PLAY:
		if (lw.selected == subcmd_item_up()) {
			screen.OnCommand(c, Command::GO_PARENT_DIRECTORY);
		} else if (lw.selected == subcmd_item_add()) {
			AddKey();
		} else {
			/* just to be sure ;-) */
			assert(subcmd_item_is_key(lw.selected));
			OverwriteKey(subcmd_item_to_key_id(lw.selected));
		}
		return true;
	case Command::DELETE:
		if (subcmd_item_is_key(lw.selected))
			DeleteKey(subcmd_item_to_key_id(lw.selected));

		return true;
	case Command::ADD:
		AddKey();
		return true;
	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, &lw, cmd, *this);
		SetDirty();
		return true;

	default:
		return false;
	}

	/* unreachable */
	assert(0);
	return false;
}

class CommandListPage final : public ListPage, ListText {
	ScreenManager &screen;

	KeyBindings *bindings;

	/** the number of commands */
	static constexpr size_t command_n_commands = size_t(Command::NONE);

public:
	CommandListPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size), screen(_screen) {}

	~CommandListPage() override {
		delete bindings;
	}

	KeyBindings *GetBindings() {
		return bindings;
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
	static constexpr unsigned command_item_apply() {
		return command_n_commands;
	}

	/** the position of the "apply and save" item */
	static constexpr unsigned command_item_save() {
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

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const override;

private:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const override;
};

bool
CommandListPage::IsModified() const
{
	const auto &orginal_bindings = GetGlobalKeyBindings();
	constexpr size_t size = sizeof(orginal_bindings);

	return memcmp(&orginal_bindings, &bindings, size) != 0;
}

void
CommandListPage::Apply()
{
	if (IsModified()) {
		auto &orginal_bindings = GetGlobalKeyBindings();
		orginal_bindings = *bindings;
		screen_status_message(_("You have new key bindings"));
	} else
		screen_status_message(_("Keybindings unchanged."));
}

void
CommandListPage::Save()
{
	std::string filename;
	if (options.key_file.empty()) {
		filename = MakeKeysPath();
		if (filename.empty()) {
			screen_status_message(_("Unable to write configuration"));
			screen_bell();
			return;
		}
	} else
		filename = options.key_file;

	FILE *f = fopen(filename.c_str(), "w");
	if (f == nullptr) {
		screen_status_printf("%s: %s - %s", _("Error"),
				     filename.c_str(), strerror(errno));
		screen_bell();
		return;
	}

	if (GetGlobalKeyBindings().WriteToFile(f, KEYDEF_WRITE_HEADER))
		screen_status_printf(_("Wrote %s"), filename.c_str());
	else
		screen_status_printf("%s: %s - %s", _("Error"),
				     filename.c_str(), strerror(errno));

	fclose(f);
}

const char *
CommandListPage::GetListItemText(char *buffer, size_t size, unsigned idx) const
{
	if (idx == command_item_apply())
		return _("===> Apply key bindings ");
	if (idx == command_item_save())
		return _("===> Apply & Save key bindings  ");

	assert(idx < command_n_commands);

	/*
	 * Format the lines in two aligned columnes for the key name and
	 * the description, like this:
	 *
	 *	this-command - do this
	 *	that-one     - do that
	 */
	const char *name = get_key_command_name(Command(idx));
	size_t len = strlen(name);
	strncpy(buffer, name, size);

	if (len < get_cmds_max_name_width())
		memset(buffer + len, ' ', get_cmds_max_name_width() - len);

	snprintf(buffer + get_cmds_max_name_width(),
		 size - get_cmds_max_name_width(),
		 " - %s", gettext(get_command_definitions()[idx].description));

	return buffer;
}

void
CommandListPage::OnOpen(gcc_unused struct mpdclient &c)
{
	if (bindings == nullptr)
		bindings = new KeyBindings(GetGlobalKeyBindings());

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
	lw.Paint(TextListRenderer(*this));
}

bool
CommandListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (cmd == Command::LIST_RANGE_SELECT)
		return false;

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case Command::PLAY:
		if (lw.selected == command_item_apply()) {
			Apply();
			return true;
		} else if (lw.selected == command_item_save()) {
			Apply();
			Save();
			return true;
		}

		break;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, &lw, cmd, *this);
		SetDirty();
		return true;

	default:
		break;
	}

	return false;
}

class KeyDefPage final : public ProxyPage {
	CommandListPage command_list_page;
	CommandKeysPage command_keys_page;

public:
	KeyDefPage(ScreenManager &screen, WINDOW *_w, Size size)
		:ProxyPage(_w),
		 command_list_page(screen, _w, size),
		 command_keys_page(screen, _w, size) {}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void OnClose() override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
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
		screen_status_message(_("Note: Did you forget to \'Apply\' your changes?"));

	ProxyPage::OnClose();
}

bool
KeyDefPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ProxyPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case Command::PLAY:
		if (GetCurrentPage() == &command_list_page) {
			int s = command_list_page.GetSelectedCommand();
			if (s >= 0) {
				command_keys_page.SetCommand(command_list_page.GetBindings(),
							     s);
				SetCurrentPage(c, &command_keys_page);
				return true;
			}
		}

		break;

	case Command::GO_PARENT_DIRECTORY:
	case Command::GO_ROOT_DIRECTORY:
		if (GetCurrentPage() != &command_list_page) {
			SetCurrentPage(c, &command_list_page);
			return true;
		}

		break;

	case Command::SAVE_PLAYLIST:
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

const PageMeta screen_keydef = {
	"keydef",
	N_("Keys"),
	Command::SCREEN_KEYDEF,
	keydef_init,
};
