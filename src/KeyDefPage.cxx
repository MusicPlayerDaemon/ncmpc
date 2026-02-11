// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "KeyDefPage.hxx"
#include "PageMeta.hxx"
#include "screen.hxx"
#include "KeyName.hxx"
#include "i18n.h"
#include "ConfigFile.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "Options.hxx"
#include "page/ListPage.hxx"
#include "page/ProxyPage.hxx"
#include "dialogs/KeyDialog.hxx"
#include "ui/Bell.hxx"
#include "ui/ListText.hxx"
#include "ui/TextListRenderer.hxx"
#include "lib/fmt/ToSpan.hxx"

#include <assert.h>
#include <errno.h>
#include <string.h>

using std::string_view_literals::operator""sv;

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
	CommandKeysPage(PageContainer &_container, ScreenManager &_screen, Page *_parent,
			const Window _window) noexcept
		:ListPage(_container, _window), screen(_screen), parent(_parent) {}

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
	[[nodiscard]]
	Co::InvokeTask OverwriteKey(int key_index) noexcept;

	/**
	 * Assign a new key to a new slot.
	 */
	void AddKey();

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

private:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
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

	Alert(_("Deleted"));

	/* repaint */
	SchedulePaint();

	/* update key conflict flags */
	bindings->Check(nullptr, 0);
}

Co::InvokeTask
CommandKeysPage::OverwriteKey(int key_index) noexcept
{
	assert(key_index < MAX_COMMAND_KEYS);

	const auto prompt = fmt::format(fmt::runtime(_("Enter new key for {}")),
					get_key_command_name(Command(subcmd)));
	const int key = co_await KeyDialog{screen, prompt};

	if (key == ERR) {
		Alert(_("Aborted"));
		co_return;
	}

	if (key == '\0') {
		Alert(_("Ctrl-Space can't be used"));
		co_return;
	}

	const Command cmd = bindings->FindKey(key);
	if (cmd != Command::NONE) {
		FmtAlert(_("Error: key {} is already used for {}"),
			 GetLocalizedKeyName(key),
			 get_key_command_name(cmd));
		Bell();
		co_return;
	}

	binding->keys[key_index] = key;
	binding->modified = true;

	FmtAlert(_("Assigned {} to {}"),
		 GetLocalizedKeyName(key),
		 get_key_command_name(Command(subcmd)));
	UpdateListLength();

	/* repaint */
	SchedulePaint();

	/* update key conflict flags */
	bindings->Check(nullptr, 0);
}

void
CommandKeysPage::AddKey()
{
	if (n_keys < MAX_COMMAND_KEYS)
		CoStart(OverwriteKey(n_keys));
}

std::string_view
CommandKeysPage::GetListItemText(std::span<char> buffer,
				 unsigned idx) const noexcept
{
	if (idx == GetLeavePosition())
		return "[..]"sv;

	if (idx == GetAddPosition())
		return FmtTruncate(buffer, "{}. {}"sv, idx, _("Add new key"));

	assert(IsKeyPosition(idx));

	return FmtTruncate(buffer, "{}. {:<20}   ({}) "sv, idx,
			   GetLocalizedKeyName(binding->keys[PositionToKeyIndex(idx)]),
			   binding->keys[PositionToKeyIndex(idx)]);
}

void
CommandKeysPage::OnOpen([[maybe_unused]] struct mpdclient &c) noexcept
{
	// TODO
}

std::string_view
CommandKeysPage::GetTitle(std::span<char> buffer) const noexcept
{
	return FmtTruncate(buffer, _("Edit keys for {}"),
			   get_key_command_name(Command(subcmd)));
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
			CoStart(OverwriteKey(PositionToKeyIndex(lw.GetCursorIndex())));
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
		CoStart(screen.find_support.Find(lw, *this, cmd));
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
	CommandListPage(PageContainer &_container, ScreenManager &_screen, const Window _window)
		:ListPage(_container, _window), screen(_screen) {}

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
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

private:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
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
		Alert(_("You have new key bindings"));
	} else
		Alert(_("Keybindings unchanged."));
}

void
CommandListPage::Save()
{
	std::string filename;
	if (options.key_file.empty()) {
		filename = MakeKeysPath();
		if (filename.empty()) {
			Alert(_("Unable to write configuration"));
			Bell();
			return;
		}
	} else
		filename = options.key_file;

	FILE *f = fopen(filename.c_str(), "w");
	if (f == nullptr) {
		Alert(fmt::format("{}: {} - {}"sv, _("Error"),
				  filename, strerror(errno)));
		Bell();
		return;
	}

	if (GetGlobalKeyBindings().WriteToFile(f, KEYDEF_WRITE_HEADER))
		FmtAlert(_("Wrote {}"), filename);
	else
		FmtAlert("{}: {} - {}"sv, _("Error"), filename, strerror(errno));

	fclose(f);
}

std::string_view
CommandListPage::GetListItemText(std::span<char> buffer,
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
	strncpy(buffer.data(), name, buffer.size());

	if (len < get_cmds_max_name_width())
		memset(buffer.data() + len, ' ', get_cmds_max_name_width() - len);

	std::size_t length = FmtTruncate(buffer.subspan(get_cmds_max_name_width()),
					 " - {}"sv, my_gettext(get_command_definitions()[idx].description)).size();

	return {buffer.data(), get_cmds_max_name_width() + length};
}

void
CommandListPage::OnOpen([[maybe_unused]] struct mpdclient &c) noexcept
{
	if (bindings == nullptr)
		bindings = new KeyBindings(GetGlobalKeyBindings());

	lw.SetLength(command_length());
}

std::string_view
CommandListPage::GetTitle(std::span<char>) const noexcept
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
		CoStart(screen.find_support.Find(lw, *this, cmd));
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
	KeyDefPage(ScreenManager &screen, const Window _window)
		:ProxyPage(screen, _window),
		 command_list_page(*this, screen, _window),
		 command_keys_page(*this, screen, this, _window) {}

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void OnClose() noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
};

static std::unique_ptr<Page>
keydef_init(ScreenManager &screen, const Window window)
{
	return std::make_unique<KeyDefPage>(screen, window);
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
		Alert(_("Note: Did you forget to \'Apply\' your changes?"));

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
