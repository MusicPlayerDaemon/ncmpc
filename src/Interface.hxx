// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string>

/**
 * An abstract interface that provides access to generic and global
 * user interface components.
 */
class Interface {
public:
	/**
	 * Show a message in the status bar.
	 */
	virtual void Alert(std::string message) noexcept = 0;
};
