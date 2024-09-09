// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "GlobalBindings.hxx"
#include "Bindings.hxx"
#include "ui/Keys.hxx"
#include "config.h"

#include <curses.h>

#define ESC  0x1B
#define RET  '\r'

static KeyBindings global_key_bindings{{{
#ifdef ENABLE_KEYDEF_SCREEN
	{'K'},
#endif
	{'q', 'Q', KEY_CTL('C')},

	/* movement */
	{KEY_UP, 'k'},
	{KEY_DOWN, 'j'},
	{'H'},
	{'M'},
	{'L'},
	{KEY_HOME, KEY_CTL('A')},
	{KEY_END, KEY_CTL('E')},
	{KEY_NPAGE},
	{KEY_PPAGE},
	{'v',  0},
	{KEY_CTL('N'),  0},
	{KEY_CTL('B'),  0},
	{'N',  0},
	{'B',  0},
	{'l'},

	/* basic screens */
	{'1', KEY_F(1), 'h'},
	{'2', KEY_F(2)},
	{'3', KEY_F(3)},

	/* player commands */
	{RET},
	{'P'},
	{'s', KEY_BACKSPACE},
	{'o'},
	{'>'},
	{'<'},
	{'f'},
	{'b'},
	{'+', KEY_RIGHT},
	{'-', KEY_LEFT},
	{' '},
	{'t'},
	{KEY_DC, 'd'},
	{'Z'},
	{'c'},
	{'r'},
	{'z'},
	{'y'},
	{'C'},
	{'x'},
	{KEY_CTL('U')},
	{'S'},
	{'a'},

	{'!'},
	{'"'},

	{'G'},

	/* lists */
	{KEY_CTL('K')},
	{KEY_CTL('J')},
	{KEY_CTL('L')},


	/* ncmpc options */
	{'w'},
	{'U'},

	/* change screen */
	{KEY_TAB},
	{KEY_STAB},
	{'`'},


	/* find */
	{'/'},
	{'n'},
	{'?'},
	{'p'},
	{'.'},


	/* extra screens */
#ifdef ENABLE_LIBRARY_PAGE
	{'4', KEY_F(4)},
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{'5', KEY_F(5)},
	{'m'},
#endif
#ifdef ENABLE_PLAYLIST_EDITOR
	{KEY_CTL('P')},
#endif
#ifdef ENABLE_SONG_SCREEN
	{'i'},
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{'7', KEY_F(7)},
	{ESC},
	{'u'},
#endif
#if defined(ENABLE_LYRICS_SCREEN) || defined(ENABLE_PLAYLIST_EDITOR)
	{'e'},
#endif

#ifdef ENABLE_OUTPUTS_SCREEN
	{'8', KEY_F(8)},
#endif

#ifdef ENABLE_CHAT_SCREEN
	{'9', KEY_F(9)},
#endif
}}};

KeyBindings &
GetGlobalKeyBindings() noexcept
{
	return global_key_bindings;
}
