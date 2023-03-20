// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef URI_UTIL_HXX
#define URI_UTIL_HXX

#include <string>

/**
 * Determins the last segment of the given URI path, i.e. the portion
 * after the last slash.  May return an empty string if the URI ends
 * with a slash.
 */
[[gnu::pure]]
const char *
GetUriFilename(const char *uri) noexcept;

/**
 * Return the "parent directory" of the given URI path, i.e. the
 * portion up to the last (forward) slash.  Returns an empty string if
 * there is no parent.
 */
[[gnu::pure]]
std::string_view
GetParentUri(std::string_view uri) noexcept;

#endif
