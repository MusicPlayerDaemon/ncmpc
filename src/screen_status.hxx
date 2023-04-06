// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_SCREEN_STATUS_H
#define NCMPC_SCREEN_STATUS_H

#include <exception>

void
screen_status_message(const char *msg) noexcept;

#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
void
screen_status_printf(const char *format, ...) noexcept;

void
screen_status_error(std::exception_ptr e) noexcept;

#endif
