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
#include "command.hxx"
#include "Compiler.h"

#include <array>

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
};

/* write key bindings flags */
#define KEYDEF_WRITE_HEADER  0x01
#define KEYDEF_WRITE_ALL     0x02
#define KEYDEF_COMMENT_ALL   0x04

gcc_pure
command_t
find_key_command(const KeyBinding *bindings, int key);

#ifndef NCMPC_MINI

/**
 * @return true on success, false on error
 */
bool
check_key_bindings(KeyBinding *bindings, char *buf, size_t size);

/**
 * @return true on success, false on error
 */
bool
write_key_bindings(FILE *f, const KeyBinding *bindings, int all);

#endif

gcc_pure
const char *
get_key_names(const KeyBinding *bindings, command_t command, bool all);

void
assign_keys(KeyBinding *bindings, command_t command,
	    const std::array<int, MAX_COMMAND_KEYS> &keys);

#endif
