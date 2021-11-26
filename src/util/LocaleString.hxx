/*
 * Copyright 2018 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
