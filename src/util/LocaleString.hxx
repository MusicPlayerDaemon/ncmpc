// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "util/StringSplit.hxx"

#include <cstddef>
#include <string_view>

/**
 * Is the given character incomplete?
 */
[[gnu::pure]]
bool
IsIncompleteCharMB(std::string_view s) noexcept;

/**
 * Returns the length of the given locale (multi-byte) string in
 * characters.
 */
[[gnu::pure]]
std::size_t
StringLengthMB(std::string_view s) noexcept;

/**
 * Wrapper for std::mbrlen() which attempts to recover with a best
 * effort from invalid or incomplete sequences.
 */
[[gnu::pure]]
std::size_t
CharSizeMB(std::string_view s) noexcept;

/**
 * Find the start of the last multi-byte character in the given
 * string.
 */
[[gnu::pure]]
const char *
LastCharMB(std::string_view s) noexcept;

/**
 * Find the #i'th character of the given string.
 *
 * @param s the string
 * @param i the character number
 *
 * @return a pointer to the first byte of the character (or the end of
 * the string if the string is shorter than #i characters)
 */
[[gnu::pure]]
const char *
AtCharMB(std::string_view s, size_t i) noexcept;

inline std::pair<std::string_view, std::string_view>
SplitAtCharMB(std::string_view s, std::size_t i) noexcept
{
	const char *p = AtCharMB(s, i);
	return Partition(s, p - s.data());
}

/**
 * Returns the number of terminal cells occupied by this multi-byte
 * string.
 */
[[gnu::pure]]
size_t
StringWidthMB(std::string_view s) noexcept;

/**
 * Find the first character which doesn't fully fit into the given width.
 *
 * @param s the string
 */
[[gnu::pure]]
const char *
AtWidthMB(std::string_view s, size_t width) noexcept;

/**
 * Return a version of the given string truncated at the specified
 * width.
 *
 * @param s the string
 * @param width the maximum width in terminal cells
 */
[[gnu::pure]]
inline std::string_view
TruncateAtWidthMB(std::string_view s, std::size_t width) noexcept
{
	const char *end = AtWidthMB(s, width);
	return {s.data(), end};
}
