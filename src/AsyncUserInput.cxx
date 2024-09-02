// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "config.h"
#include "AsyncUserInput.hxx"
#include "Command.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "ncmpc.hxx"
#include "ui/Point.hxx"

static constexpr bool
ignore_key(int key) noexcept
{
	return key == ERR || key == '\0';
}

[[gnu::pure]]
static Command
translate_key(int key) noexcept
{
	return GetGlobalKeyBindings().FindKey(key);
}

void
AsyncUserInput::OnSocketReady(unsigned) noexcept
{
	int key = wgetch(&w);
	if (ignore_key(key))
		return;

#ifdef HAVE_GETMOUSE
	if (key == KEY_MOUSE) {
		MEVENT event;

		/* retrieve the mouse event from curses */
#ifdef PDCURSES
		nc_getmouse(&event);
#else
		getmouse(&event);
#endif

		begin_input_event();
		do_mouse_event({event.x, event.y}, event.bstate);
		end_input_event();

		return;
	}
#endif

	Command cmd = translate_key(key);
	if (cmd == Command::NONE)
		return;

	begin_input_event();

	if (!do_input_event(stdin_event.GetEventLoop(), cmd))
		return;

	end_input_event();
	return;
}

AsyncUserInput::AsyncUserInput(EventLoop &event_loop, WINDOW &_w) noexcept
	:stdin_event(event_loop, BIND_THIS_METHOD(OnSocketReady),
		      FileDescriptor{STDIN_FILENO}),
	 w(_w)
{
	stdin_event.ScheduleRead();
}

void
keyboard_unread(EventLoop &event_loop, int key) noexcept
{
	if (ignore_key(key))
		return;

	Command cmd = translate_key(key);
	if (cmd != Command::NONE)
		do_input_event(event_loop, cmd);
}
