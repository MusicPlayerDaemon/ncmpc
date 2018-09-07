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

#ifndef BINDINGS_HXX
#define BINDINGS_HXX

#include "config.h"
#include "Command.hxx"
#include "Compiler.h"

#include <array>
#include <algorithm>

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

	gcc_pure
	bool HasKey(int key) const {
		return std::find(keys.begin(), keys.end(), key) != keys.end();
	}

	void SetKey(const std::array<int, MAX_COMMAND_KEYS> &_keys) {
		keys = _keys;
#ifndef NCMPC_MINI
		modified = true;
#endif
	}

#ifndef NCMPC_MINI
	void WriteToFile(FILE *f, const command_definition_t &cmd,
			 bool comment) const;
#endif
};

struct KeyBindings {
	std::array<KeyBinding, size_t(CMD_NONE)> key_bindings;

	gcc_pure
	command_t FindKey(int key) const;

	gcc_pure
	const char *GetKeyNames(command_t command, bool all) const;

	void SetKey(command_t command,
		    const std::array<int, MAX_COMMAND_KEYS> &keys) {
		auto &b = key_bindings[size_t(command)];
		b.SetKey(keys);
	}

#ifndef NCMPC_MINI
	/**
	 * @return true on success, false on error
	 */
	bool Check(char *buf, size_t size) const;

	/**
	 * @return true on success, false on error
	 */
	bool WriteToFile(FILE *f, int all) const;
#endif
};

/* write key bindings flags */
#define KEYDEF_WRITE_HEADER  0x01
#define KEYDEF_WRITE_ALL     0x02
#define KEYDEF_COMMENT_ALL   0x04

#endif
