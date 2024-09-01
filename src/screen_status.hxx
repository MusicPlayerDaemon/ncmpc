// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <exception>
#include <string>

void
screen_status_message(const char *msg) noexcept;

void
screen_status_message(std::string &&msg) noexcept;

#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
void
screen_status_printf(const char *format, ...) noexcept;

void
screen_status_error(std::exception_ptr e) noexcept;
