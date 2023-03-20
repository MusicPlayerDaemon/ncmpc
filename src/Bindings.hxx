// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef BINDINGS_HXX
#define BINDINGS_HXX

#include "config.h" // IWYU pragma: keep
#include "Command.hxx"

#include <array>
#include <algorithm>
#include <string>

#include <stddef.h>

#ifndef NCMPC_MINI
#include <stdio.h>
#endif

#define MAX_COMMAND_KEYS 3

struct KeyBinding {
	std::array<int, MAX_COMMAND_KEYS> keys;

#ifndef NCMPC_MINI
	bool modified = false;
#endif

	constexpr KeyBinding(int a, int b=0, int c=0) noexcept
		:keys{{a, b, c}} {}

	[[gnu::pure]]
	bool HasKey(int key) const noexcept {
		return std::find(keys.begin(), keys.end(), key) != keys.end();
	}

	[[gnu::pure]]
	size_t GetKeyCount() const noexcept {
		return std::distance(keys.begin(),
				     std::find(keys.begin(), keys.end(), 0));
	}

	void SetKey(const std::array<int, MAX_COMMAND_KEYS> &_keys) noexcept {
		keys = _keys;
#ifndef NCMPC_MINI
		modified = true;
#endif
	}

#ifndef NCMPC_MINI
	void WriteToFile(FILE *f, const command_definition_t &cmd,
			 bool comment) const noexcept;
#endif
};

struct KeyBindings {
	std::array<KeyBinding, size_t(Command::NONE)> key_bindings;

	[[gnu::pure]]
	Command FindKey(int key) const noexcept;

	/**
	 * Returns the name of the first key bound to the given
	 * command, or nullptr if there is no key binding.
	 */
	[[gnu::pure]]
	const char *GetFirstKeyName(Command command) const noexcept;

	[[gnu::pure]]
	std::string GetKeyNames(Command command) const noexcept;

	void SetKey(Command command,
		    const std::array<int, MAX_COMMAND_KEYS> &keys) noexcept {
		auto &b = key_bindings[size_t(command)];
		b.SetKey(keys);
	}

#ifndef NCMPC_MINI
	/**
	 * @return true on success, false on error
	 */
	bool Check(char *buf, size_t size) const noexcept;

	/**
	 * @return true on success, false on error
	 */
	bool WriteToFile(FILE *f, int all) const noexcept;
#endif
};

/* write key bindings flags */
#define KEYDEF_WRITE_HEADER  0x01
#define KEYDEF_WRITE_ALL     0x02
#define KEYDEF_COMMENT_ALL   0x04

#endif
