// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ncu.hxx"
#include "ui/Keys.hxx"
#include "config.h"

#ifdef ENABLE_COLORS
#include "Styles.hxx"
#endif

#ifdef HAVE_GETMOUSE
#include "Options.hxx"
#endif

#include <curses.h>

#include <stdexcept>

static SCREEN *ncu_screen;

void
ncu_init()
{
	/* initialize the curses library */
	ncu_screen = newterm(nullptr, stdout, stdin);
	if (ncu_screen == nullptr)
		throw std::runtime_error{"Failed to initialize terminal"};

	/* initialize color support */
#ifdef ENABLE_COLORS
	ApplyStyles();
#endif

	/* Ctrl-C generates keycode 0x03 instead of SIGINT */
	raw();

	/* tell curses not to do NL->CR/NL on output */
	nonl();

	/* don't echo input */
	noecho();

	/* set cursor invisible */
	curs_set(0);

	/* enable extra keys */
	keypad(stdscr, true);

	InitKeys();

	/* initialize mouse support */
#ifdef HAVE_GETMOUSE
	if (options.enable_mouse)
		mousemask(ALL_MOUSE_EVENTS, nullptr);
#endif

	refresh();
}

void
ncu_deinit()
{
	endwin();

	delscreen(ncu_screen);
}
