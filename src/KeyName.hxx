// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef KEY_NAME_HXX
#define KEY_NAME_HXX

#include <utility>

/**
 * Parse a string (from the configuration file) to a keycode.
 *
 * @return the keycode and the first unparsed character; -1 indicates
 * error
 */
[[gnu::pure]]
std::pair<int, const char *>
ParseKeyName(const char *s) noexcept;

/**
 * Convert a keycode to a canonical string, to be used in the
 * configuration file.
 *
 * The returned pointer is invalidated by the next call.
 */
[[gnu::pure]]
const char *
GetKeyName(int key) noexcept;

/**
 * Convert a keycode to a human-readable localized string.
 *
 * The returned pointer is invalidated by the next call.
 */
[[gnu::pure]]
const char *
GetLocalizedKeyName(int key) noexcept;

#endif
