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

#include "GlobalBindings.hxx"
#include "Bindings.hxx"
#include "ncmpc_curses.h"
#include "util/Macros.hxx"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>

#define KEY_CTL(x) ((x) & 0x1f) /* KEY_CTL(A) == ^A == \1 */

#define BS   KEY_BACKSPACE
#define DEL  KEY_DC
#define UP   KEY_UP
#define DWN  KEY_DOWN
#define LEFT KEY_LEFT
#define RGHT KEY_RIGHT
#define HOME KEY_HOME
#define END  KEY_END
#define PGDN KEY_NPAGE
#define PGUP KEY_PPAGE
#define TAB  0x09
#define STAB 0x161
#define ESC  0x1B
#define RET  '\r'
#define F1   KEY_F(1)
#define F2   KEY_F(2)
#define F3   KEY_F(3)
#define F4   KEY_F(4)
#define F5   KEY_F(5)
#define F6   KEY_F(6)
#define F7   KEY_F(7)
#define F8   KEY_F(8)
#define F9   KEY_F(9)
#define C(x) KEY_CTL(x)

static KeyBinding global_key_bindings[] = {
#ifdef ENABLE_KEYDEF_SCREEN
	{ {'K', 0, 0 } },
#endif
	{ { 'q', 'Q', C('C') } },

	/* movement */
	{ { UP, 'k', 0 } },
	{ { DWN, 'j', 0 } },
	{ { 'H', 0, 0 } },
	{ { 'M', 0, 0 } },
	{ { 'L', 0, 0 } },
	{ { HOME, C('A'), 0 } },
	{ { END, C('E'), 0 } },
	{ { PGUP, 0, 0 } },
	{ { PGDN, 0, 0 } },
	{ { 'v',  0, 0 } },
	{ { C('N'),  0, 0 } },
	{ { C('B'),  0, 0 } },
	{ { 'N',  0, 0 } },
	{ { 'B',  0, 0 } },
	{ { 'l', 0, 0 } },

	/* basic screens */
	{ { '1', F1, 'h' } },
	{ { '2', F2, 0 } },
	{ { '3', F3, 0 } },

	/* player commands */
	{ { RET, 0, 0 } },
	{ { 'P', 0, 0 } },
	{ { 's', BS, 0 } },
	{ { 'o', 0, 0 } },
	{ { '>', 0, 0 } },
	{ { '<', 0, 0 } },
	{ { 'f', 0, 0 } },
	{ { 'b', 0, 0 } },
	{ { '+', RGHT, 0 } },
	{ { '-', LEFT, 0 } },
	{ { ' ', 0, 0 } },
	{ { 't', 0, 0 } },
	{ { DEL, 'd', 0 } },
	{ { 'Z', 0, 0 } },
	{ { 'c', 0, 0 } },
	{ { 'r', 0, 0 } },
	{ { 'z', 0, 0 } },
	{ { 'y', 0, 0 } },
	{ { 'C', 0, 0 } },
	{ { 'x', 0, 0 } },
	{ { C('U'), 0, 0 } },
	{ { 'S', 0, 0 } },
	{ { 'a', 0, 0 } },

	{ { '!', 0, 0 } },
	{ { '"', 0, 0 } },

	{ { 'G', 0, 0 } },

	/* lists */
	{ { C('K'), 0, 0 } },
	{ { C('J'), 0, 0 } },
	{ { C('L'), 0, 0 } },


	/* ncmpc options */
	{ { 'w', 0, 0 } },
	{ { 'U', 0, 0 } },

	/* change screen */
	{ { TAB, 0, 0 } },
	{ { STAB, 0, 0 } },
	{ { '`', 0, 0 } },


	/* find */
	{ { '/', 0, 0 } },
	{ { 'n', 0, 0 } },
	{ { '?', 0, 0 } },
	{ { 'p', 0, 0 } },
	{ { '.', 0, 0 } },


	/* extra screens */
#ifdef ENABLE_ARTIST_SCREEN
	{ {'4', F4, 0 } },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ {'5', F5, 0 } },
	{ {'m', 0, 0 } },
#endif
#ifdef ENABLE_SONG_SCREEN
	{ { 'i', 0, 0 } },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ {'7', F7, 0 } },
	{ {ESC, 0, 0 } },
	{ {'u', 0, 0 } },
	{ {'e', 0, 0 } },
#endif

#ifdef ENABLE_OUTPUTS_SCREEN
	{ {'8', F8, 0 } },
#endif

#ifdef ENABLE_CHAT_SCREEN
	{ {'9', F9, 0} },
#endif
};

static_assert(ARRAY_SIZE(global_key_bindings) == size_t(CMD_NONE),
	      "Wrong key binding table size");

KeyBinding *
GetGlobalKeyBindings()
{
	return global_key_bindings;
}
