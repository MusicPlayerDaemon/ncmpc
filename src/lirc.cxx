// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "lirc.hxx"
#include "UserInputHandler.hxx"
#include "ncmpc.hxx"
#include "Command.hxx"
#include "config.h"

#include <lirc/lirc_client.h>

void
LircInput::OnSocketReady(unsigned) noexcept
{
	char *code, *txt;

	if (lirc_nextcode(&code) == 0) {
		while (lirc_code2char(lc, code, &txt) == 0 && txt != nullptr) {
			const auto cmd = get_key_command_from_name(txt);
			if (!handler.OnCommand(cmd))
				return;
		}
	}

	end_input_event();
}

LircInput::LircInput(EventLoop &_event_loop,
		     UserInputHandler &_handler) noexcept
	:handler(_handler),
	 event(_event_loop, BIND_THIS_METHOD(OnSocketReady))
{
	int lirc_socket = 0;

	if ((lirc_socket = lirc_init(PACKAGE, 0)) == -1)
		return;

	if (lirc_readconfig(nullptr, &lc, nullptr)) {
		lirc_deinit();
		return;
	}

	event.Open(SocketDescriptor(lirc_socket));
	event.ScheduleRead();
}

LircInput::~LircInput()
{
	if (lc)
		lirc_freeconfig(lc);
	if (event.IsDefined())
		lirc_deinit();
}
