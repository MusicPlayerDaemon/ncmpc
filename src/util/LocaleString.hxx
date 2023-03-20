// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#ifndef LOCALE_STRING_HXX
#define LOCALE_STRING_HXX

#include <cstddef>

/**
 * Is the given character incomplete?
 */
[[gnu::pure]]
bool
IsIncompleteCharMB(const char *s, size_t n) noexcept;

/**
 * Returns the length of the given locale (multi-byte) string in
 * characters.
 */
[[gnu::pure]]
std::size_t
StringLengthMB(const char *s, size_t n) noexcept;

/**
 * Wrapper for std::mbrlen() which attempts to recover with a best
 * effort from invalid or incomplete sequences.
 */
[[gnu::pure]]
std::size_t
CharSizeMB(const char *s, size_t n) noexcept;

/**
 * Determine the start of the character preceding the given reference.
 *
 * @param s the start of the string
 */
[[gnu::pure]]
const char *
PrevCharMB(const char *start, const char *reference) noexcept;

/**
 * Find the `i`th character of the given string.  Returns the end of
 * the string if the string is shorter than `i` characters.
 *
 * @param s the start of the string
 */
[[gnu::pure]]
const char *
AtCharMB(const char *s, size_t length, size_t i) noexcept;

/**
 * Returns the number of terminal cells occupied by this multi-byte
 * string.
 */
[[gnu::pure]]
size_t
StringWidthMB(const char *s, size_t length) noexcept;

[[gnu::pure]]
size_t
StringWidthMB(const char *s) noexcept;

/**
 * Find the first character which doesn't fully fit into the given width.
 *
 * @param s the start of the string
 */
[[gnu::pure]]
const char *
AtWidthMB(const char *s, size_t length, size_t width) noexcept;

#endif
