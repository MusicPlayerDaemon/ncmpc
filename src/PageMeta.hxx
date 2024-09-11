// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <memory>

enum class Command : unsigned;
class Page;
class ScreenManager;
struct Window;

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

	std::unique_ptr<Page> (*init)(ScreenManager &screen, Window window);
};
