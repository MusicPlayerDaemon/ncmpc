// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <span>
#include <string_view>

[[nodiscard]] [[gnu::pure]]
std::string_view
format_duration_short(std::span<char> buffer, unsigned duration) noexcept;

void
format_duration_long(std::span<char> buffer, unsigned long duration) noexcept;
