// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "config.h"
#include "AsyncUserInput.hxx"
#include "UserInputHandler.hxx"
#include "Command.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "ncmpc.hxx"
#include "ui/Point.hxx"

// TODO remove this kludge
static AsyncUserInput *global_async_user_input;

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

		if (!handler.OnMouse({event.x, event.y}, event.bstate))
			return;

		end_input_event();

		return;
	}
#endif

	if (handler.OnRawKey(key)) {
		// TODO: begin_input_event() ?
		end_input_event();
		return;
	}

	Command cmd = translate_key(key);
	if (cmd == Command::NONE)
		return;

	begin_input_event();

	if (!handler.OnCommand(cmd))
		return;

	end_input_event();
	return;
}

AsyncUserInput::AsyncUserInput(EventLoop &event_loop, WINDOW &_w,
			       UserInputHandler &_handler) noexcept
	:stdin_event(event_loop, BIND_THIS_METHOD(OnSocketReady),
		      FileDescriptor{STDIN_FILENO}),
	 w(_w),
	 handler(_handler)
{
	stdin_event.ScheduleRead();

	// TODO remove this kludge
	global_async_user_input = this;
}

inline void
AsyncUserInput::InjectKey(int key) noexcept
{
	if (ignore_key(key))
		return;

	if (Command cmd = translate_key(key); cmd != Command::NONE)
		handler.OnCommand(cmd);
}

void
keyboard_unread(int key) noexcept
{
	global_async_user_input->InjectKey(key);
}
