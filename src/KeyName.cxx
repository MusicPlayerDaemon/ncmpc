/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#include "KeyName.hxx"
#include "i18n.h"

#include <curses.h>

#include <stdio.h>

const char *
GetLocalizedKeyName(int key) noexcept
{
	switch(key) {
	case 0:
		return _("Undefined");
	case ' ':
		return _("Space");
	case '\r':
		return _("Enter");
	case KEY_BACKSPACE:
		return _("Backspace");
	case KEY_DC:
		return _("Delete");
	case KEY_UP:
		return _("Up");
	case KEY_DOWN:
		return _("Down");
	case KEY_LEFT:
		return _("Left");
	case KEY_RIGHT:
		return _("Right");
	case KEY_HOME:
		return _("Home");
	case KEY_END:
		return _("End");
	case KEY_NPAGE:
		return _("PageDown");
	case KEY_PPAGE:
		return _("PageUp");
	case '\t':
		return _("Tab");
	case KEY_BTAB:
		return _("Shift+Tab");
	case '\x1b':
		return _("Esc");
	case KEY_IC:
		return _("Insert");
	}

	static char buf[32];

	for (int i = 0; i <= 63; i++) {
		if (key == KEY_F(i)) {
			snprintf(buf, sizeof(buf), "F%d", i );
			return buf;
		}
	}

	const char *name = keyname(key);
	if (name == nullptr) {
		snprintf(buf, sizeof(buf), "0x%03X", key);
		return buf;
	}

	if (name[0] == '^' && name[1] != 0 && name[2] == 0) {
		/* translate "^X" to "Ctrl-X" */
		snprintf(buf, sizeof(buf), _("Ctrl-%c"), name[1]);
		return buf;
	}

	if (name[0] == 'M' && name[1] == '-' && name[2] != 0 && name[3] == 0) {
		/* translate "M-X" to "Alt-X" */
		snprintf(buf, sizeof(buf), _("Alt-%c"), name[2]);
		return buf;
	}

	return name;
}
