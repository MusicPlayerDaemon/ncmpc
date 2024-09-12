// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "Completion.hxx"

struct Window;

void
screen_display_completion_list(Window window,
			       std::string_view prefix,
			       Completion::Range range) noexcept;
