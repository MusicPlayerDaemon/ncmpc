// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "StringEditDialog.hxx"

Co::Task<bool>
StringEditDialog(ModalDock &dock,
		 std::string_view prompt,
		 std::string &value,
		 TextInputDialogOptions options) noexcept
{
	TextInputDialog dialog{
		dock, prompt, std::string{value}, options,
	};

	auto new_value = co_await dialog;

	if (dialog.WasCanceled())
		co_return false;

	value = std::move(new_value);
	co_return true;
}

