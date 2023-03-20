// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef TIME_FORMAT_H
#define TIME_FORMAT_H

#include <stddef.h>

void
format_duration_short(char *buffer, size_t length, unsigned duration) noexcept;

void
format_duration_long(char *buffer, size_t length, unsigned long duration) noexcept;

#endif
