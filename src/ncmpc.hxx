// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

class ScreenManager;
extern ScreenManager *screen;

void
begin_input_event() noexcept;

void
end_input_event() noexcept;
