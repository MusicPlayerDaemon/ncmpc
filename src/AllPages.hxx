// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

enum class Command : unsigned;
struct PageMeta;

[[gnu::const]]
const PageMeta *
GetPageMeta(unsigned i) noexcept;

[[gnu::pure]]
const PageMeta *
screen_lookup_name(const char *name) noexcept;

[[gnu::const]]
const PageMeta *
PageByCommand(Command cmd) noexcept;
