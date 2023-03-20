// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_H
#define NCMPC_H

#ifdef HAVE_GETMOUSE
#include <curses.h>
#endif

enum class Command : unsigned;
struct Point;
class EventLoop;
class ScreenManager;
extern ScreenManager *screen;

void
begin_input_event() noexcept;

void
end_input_event() noexcept;

/**
 * @return false if the application shall quit
 */
bool
do_input_event(EventLoop &event_loop, Command cmd) noexcept;

#ifdef HAVE_GETMOUSE
void
do_mouse_event(Point p, mmask_t bstate) noexcept;
#endif

#endif /* NCMPC_H */
