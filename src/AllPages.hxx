// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <string_view>

enum class Command : unsigned;
struct PageMeta;

/**
 * A nullptr-terminated list of #PageMeta instances.
 */
extern const PageMeta *const all_pages[];

[[gnu::pure]]
const PageMeta *
screen_lookup_name(std::string_view name) noexcept;

[[gnu::const]]
const PageMeta *
PageByCommand(Command cmd) noexcept;
