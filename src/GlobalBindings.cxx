// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "GlobalBindings.hxx"
#include "Bindings.hxx"
#include "ui/Keys.hxx"
#include "config.h"

#include <curses.h>

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
