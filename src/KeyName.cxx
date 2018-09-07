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

#include "KeyName.hxx"
#include "i18n.h"
#include "ncmpc_curses.h"

const char *
key2str(int key)
{
	switch(key) {
		static char buf[32];

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
	default:
		for (int i = 0; i <= 63; i++)
			if (key == KEY_F(i)) {
				snprintf(buf, 32, "F%d", i );
				return buf;
			}
		if (!(key & ~037))
			snprintf(buf, 32, _("Ctrl-%c"), 'A'+(key & 037)-1 );
		else if ((key & ~037) == 224)
			snprintf(buf, 32, _("Alt-%c"), 'A'+(key & 037)-1 );
		else if (key > 32 && key < 256)
			snprintf(buf, 32, "%c", key);
		else
			snprintf(buf, 32, "0x%03X", key);
		return buf;
	}
}
