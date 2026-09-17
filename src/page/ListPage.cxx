// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ListPage.hxx"
#include "ui/dialogs/TextInputDialog.hxx"
#include "co/InvokeTask.hxx"
#include "i18n.h"

#include <fmt/format.h>

Co::InvokeTask
ListPage::Jump(ModalDock &modal_dock, const ListText &text) noexcept
{
	TextInputDialog dialog{
		modal_dock,
		_("Jump"),
		{},
		{ .fragile = true },
	};

	dialog.SetModifiedCallback([this, &text](std::string_view value) noexcept {
		lw.Jump(text, value);
		SchedulePaint();
	});

	co_await dialog;
}
