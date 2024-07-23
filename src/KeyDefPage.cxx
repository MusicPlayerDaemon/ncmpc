// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "KeyDefPage.hxx"
#include "PageMeta.hxx"
#include "ListPage.hxx"
#include "ListText.hxx"
#include "TextListRenderer.hxx"
#include "ProxyPage.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "KeyName.hxx"
#include "i18n.h"
#include "ConfigFile.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "screen_utils.hxx"
#include "Options.hxx"

#include <assert.h>
#include <errno.h>
#include <string.h>

class CommandKeysPage final : public ListPage, ListText {
	ScreenManager &screen;
	Page *const parent;

	const KeyBindings *bindings;
	KeyBinding *binding;

	/**
	 * The command being edited, represented by a array subscript
	 * to #bindings, or -1, if no command is being edited
	 */
	int subcmd = -1;

	/** The number of keys assigned to the current command */
	unsigned n_keys = 0;

public:
	CommandKeysPage(ScreenManager &_screen, Page *_parent,
			WINDOW *w, Size size) noexcept
		:ListPage(w, size), screen(_screen), parent(_parent) {}

	void SetCommand(KeyBindings *_bindings, unsigned _cmd) {
		bindings = _bindings;
		binding = &_bindings->key_bindings[_cmd];
		subcmd = _cmd;
		lw.Reset();
		UpdateListLength();
	}

private:
	/** The position of the up ("[..]") item */
	static constexpr unsigned GetLeavePosition() {
		return 0;
	}

	/** The position of the "add a key" item */
	[[gnu::pure]]
	unsigned GetAddPosition() const noexcept {
		return n_keys + 1;
	}

	/** The number of items in the list_window, if there's a command being edited */
	[[gnu::pure]]
	unsigned CalculateListLength() const noexcept {
		/* show "add key" only if there is room for more keys */
		return GetAddPosition() + (n_keys < MAX_COMMAND_KEYS);
	}

	/** Check whether a given item is a key */
	[[gnu::pure]]
	bool IsKeyPosition(unsigned i) const noexcept {
		return (i > GetLeavePosition() && i < GetAddPosition());
	}

	/**
	 * Convert an item id (as in lw.GetCursorIndex()) into a "key
	 * id", which is an array subscript to cmds[subcmd].keys.
	 */
	static constexpr unsigned PositionToKeyIndex(unsigned i) {
		return i - 1;
	}

	/* TODO: rename to check_n_keys / subcmd_count_keys? */
	void UpdateListLength() noexcept;

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
	void OnOpen(struct mpdclient &c) noexcept override;
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const noexcept override;

private:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const noexcept override;
};

/* TODO: rename to check_n_keys / subcmd_count_keys? */
void
CommandKeysPage::UpdateListLength() noexcept
{
	n_keys = binding->GetKeyCount();

	lw.SetLength(CalculateListLength());
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
	UpdateListLength();

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
	const int key = screen_getch(screen, prompt);

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
				     GetLocalizedKeyName(key),
				     get_key_command_name(cmd));
		screen_bell();
		return;
	}

	binding->keys[key_index] = key;
	binding->modified = true;

	screen_status_printf(_("Assigned %s to %s"),
			     GetLocalizedKeyName(key),
			     get_key_command_name(Command(subcmd)));
	UpdateListLength();

	/* repaint */
	SetDirty();

	/* update key conflict flags */
	bindings->Check(nullptr, 0);
}

void
CommandKeysPage::AddKey()
{
	if (n_keys < MAX_COMMAND_KEYS)
		OverwriteKey(n_keys);
}

const char *
CommandKeysPage::GetListItemText(char *buffer, size_t size,
				 unsigned idx) const noexcept
{
	if (idx == GetLeavePosition())
		return "[..]";

	if (idx == GetAddPosition()) {
		snprintf(buffer, size, "%d. %s", idx, _("Add new key"));
		return buffer;
	}

	assert(IsKeyPosition(idx));

	snprintf(buffer, size,
		 "%d. %-20s   (%d) ", idx,
		 GetLocalizedKeyName(binding->keys[PositionToKeyIndex(idx)]),
		 binding->keys[PositionToKeyIndex(idx)]);
	return buffer;
}

void
CommandKeysPage::OnOpen([[maybe_unused]] struct mpdclient &c) noexcept
{
	// TODO
}

const char *
CommandKeysPage::GetTitle(char *str, size_t size) const noexcept
{
	snprintf(str, size, _("Edit keys for %s"),
		 get_key_command_name(Command(subcmd)));
	return str;
}

void
CommandKeysPage::Paint() const noexcept
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
		if (lw.GetCursorIndex() == GetLeavePosition()) {
			if (parent != nullptr)
				return parent->OnCommand(c, Command::GO_PARENT_DIRECTORY);
		} else if (lw.GetCursorIndex() == GetAddPosition()) {
			AddKey();
		} else {
			/* just to be sure ;-) */
			assert(IsKeyPosition(lw.GetCursorIndex()));
			OverwriteKey(PositionToKeyIndex(lw.GetCursorIndex()));
		}
		return true;
	case Command::DELETE:
		if (IsKeyPosition(lw.GetCursorIndex()))
			DeleteKey(PositionToKeyIndex(lw.GetCursorIndex()));

		return true;
	case Command::ADD:
		AddKey();
		return true;
	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, lw, cmd, *this);
		SetDirty();
		return true;

	default:
		return false;
	}
}

class CommandListPage final : public ListPage, ListText {
	ScreenManager &screen;

	KeyBindings *bindings = nullptr;

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
		return lw.GetCursorIndex() < command_n_commands
			? (int)lw.GetCursorIndex()
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
	[[gnu::pure]]
	unsigned command_length() const {
		return command_item_save() + 1;
	}

	/** The position of the up ("[..]") item */
	static constexpr unsigned GetLeavePosition() {
		return 0;
	}

public:
	bool IsModified() const;

	void Apply();
	void Save();

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const noexcept override;

private:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const noexcept override;
};

bool
CommandListPage::IsModified() const
{
	const auto &orginal_bindings = GetGlobalKeyBindings();
	constexpr size_t size = sizeof(orginal_bindings);

	return memcmp(&orginal_bindings, bindings, size) != 0;
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
CommandListPage::GetListItemText(char *buffer, size_t size,
				 unsigned idx) const noexcept
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
		 " - %s", my_gettext(get_command_definitions()[idx].description));

	return buffer;
}

void
CommandListPage::OnOpen([[maybe_unused]] struct mpdclient &c) noexcept
{
	if (bindings == nullptr)
		bindings = new KeyBindings(GetGlobalKeyBindings());

	lw.SetLength(command_length());
}

const char *
CommandListPage::GetTitle(char *, size_t) const noexcept
{
	return _("Edit key bindings");
}

void
CommandListPage::Paint() const noexcept
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
		if (lw.GetCursorIndex() == command_item_apply()) {
			Apply();
			return true;
		} else if (lw.GetCursorIndex() == command_item_save()) {
			Apply();
			Save();
			return true;
		}

		break;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, lw, cmd, *this);
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
		 command_keys_page(screen, this, _w, size) {}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void OnClose() noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
};

static std::unique_ptr<Page>
keydef_init(ScreenManager &screen, WINDOW *w, Size size)
{
	return std::make_unique<KeyDefPage>(screen, w, size);
}

void
KeyDefPage::OnOpen(struct mpdclient &c) noexcept
{
	ProxyPage::OnOpen(c);

	if (GetCurrentPage() == nullptr)
		SetCurrentPage(c, &command_list_page);
}

void
KeyDefPage::OnClose() noexcept
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
