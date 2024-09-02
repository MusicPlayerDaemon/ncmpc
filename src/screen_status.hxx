// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <fmt/core.h>

#include <exception>
#include <string>

void
screen_status_message(const char *msg) noexcept;

void
screen_status_message(std::string_view msg) noexcept;

void
screen_status_message(std::string &&msg) noexcept;

void
screen_status_vfmt(fmt::string_view format_str, fmt::format_args args) noexcept;

template<typename S, typename... Args>
inline void
screen_status_fmt(const S &format_str, Args&&... args) noexcept
{
	screen_status_vfmt(format_str, fmt::make_format_args(args...));
}

#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
void
screen_status_printf(const char *format, ...) noexcept;

void
screen_status_error(std::exception_ptr e) noexcept;
