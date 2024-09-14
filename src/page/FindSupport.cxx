// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "FindSupport.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "Command.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "ui/Bell.hxx"
#include "ui/ListWindow.hxx"
#include "ui/Options.hxx"
#include "co/InvokeTask.hxx"

#include <fmt/core.h>

#define FIND_PROMPT  _("Find")
#define RFIND_PROMPT _("Find backward")
#define JUMP_PROMPT _("Jump")

inline Co::InvokeTask
FindSupport::DoFind(ListWindow &lw, const ListText &text, bool reversed) noexcept
{
	if (last.empty()) {
		const char *const prompt = reversed ? RFIND_PROMPT : FIND_PROMPT;

		std::string value;
		if (ui_options.find_show_last_pattern && !history.empty())
			value = history.back();

		last = co_await TextInputDialog{
			screen,
			prompt,
			std::move(value),
			&history,
		};
	}

	if (last.empty())
		co_return;

	bool found = reversed
		? lw.ReverseFind(text, last)
		: lw.Find(text, last);
	if (!found) {
		screen.Alert(fmt::format(fmt::runtime(_("Unable to find \'{}\'")),
					 last));
		Bell();
	}
}

Co::InvokeTask
FindSupport::Find(ListWindow &lw, const ListText &text, Command cmd) noexcept
{
	const bool reversed =
		cmd == Command::LIST_RFIND ||
		cmd == Command::LIST_RFIND_NEXT;

	switch (cmd) {
	case Command::LIST_FIND:
	case Command::LIST_RFIND:
		last.clear();
		/* fall through */

	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		return DoFind(lw, text, reversed);

	default:
		return {};
	}
}

Co::InvokeTask
FindSupport::Jump(ListWindow &lw,
		  const ListText &text,
		  const ListRenderer &renderer) noexcept
{
	TextInputDialog dialog{
		screen,
		JUMP_PROMPT,
	};

	dialog.SetFragile();

	dialog.SetModifiedCallback([&lw, &text, &renderer](std::string_view value) noexcept {
		lw.Jump(text, value);
		lw.Paint(renderer);
	});

	co_await dialog;
}
