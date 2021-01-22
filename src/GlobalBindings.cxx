/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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
#include "config.h"

#include <curses.h>

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

static KeyBindings global_key_bindings{{{
#ifdef ENABLE_KEYDEF_SCREEN
	{'K'},
#endif
	{'q', 'Q', C('C')},

	/* movement */
	{UP, 'k'},
	{DWN, 'j'},
	{'H'},
	{'M'},
	{'L'},
	{HOME, C('A')},
	{END, C('E')},
	{PGUP},
	{PGDN},
	{'v',  0},
	{C('N'),  0},
	{C('B'),  0},
	{'N',  0},
	{'B',  0},
	{'l'},

	/* basic screens */
	{'1', F1, 'h'},
	{'2', F2},
	{'3', F3},

	/* player commands */
	{RET},
	{'P'},
	{'s', BS},
	{'o'},
	{'>'},
	{'<'},
	{'f'},
	{'b'},
	{'+', RGHT},
	{'-', LEFT},
	{' '},
	{'t'},
	{DEL, 'd'},
	{'Z'},
	{'c'},
	{'r'},
	{'z'},
	{'y'},
	{'C'},
	{'x'},
	{C('U')},
	{'S'},
	{'a'},

	{'!'},
	{'"'},

	{'G'},

	/* lists */
	{C('K')},
	{C('J')},
	{C('L')},


	/* ncmpc options */
	{'w'},
	{'U'},

	/* change screen */
	{TAB},
	{STAB},
	{'`'},


	/* find */
	{'/'},
	{'n'},
	{'?'},
	{'p'},
	{'.'},


	/* extra screens */
#ifdef ENABLE_LIBRARY_PAGE
	{'4', F4},
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{'5', F5},
	{'m'},
#endif
#ifdef ENABLE_PLAYLIST_EDITOR
	{C('P')},
#endif
#ifdef ENABLE_SONG_SCREEN
	{'i'},
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{'7', F7},
	{ESC},
	{'u'},
#endif
#if defined(ENABLE_LYRICS_SCREEN) || defined(ENABLE_PLAYLIST_EDITOR)
	{'e'},
#endif

#ifdef ENABLE_OUTPUTS_SCREEN
	{'8', F8},
#endif

#ifdef ENABLE_CHAT_SCREEN
	{'9', F9},
#endif
}}};

KeyBindings &
GetGlobalKeyBindings() noexcept
{
	return global_key_bindings;
}
