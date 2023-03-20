// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_PAGE_META_HXX
#define NCMPC_PAGE_META_HXX

#include "Size.hxx"

#include <memory>

#include <curses.h>

enum class Command : unsigned;
class Page;
class ScreenManager;

struct PageMeta {
	const char *name;

	/**
	 * A title/caption for this page, to be translated using
	 * gettext().
	 */
	const char *title;

	/**
	 * The command which switches to this page.
	 */
	Command command;

	std::unique_ptr<Page> (*init)(ScreenManager &screen, WINDOW *w, Size size);
};

#endif
