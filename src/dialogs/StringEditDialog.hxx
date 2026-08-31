// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "TextInputDialog.hxx"
#include "co/Task.hxx"

/**
 * Ask the user to enter text.
 *
 * @param value in/out: old value and result value
 *
 * @return true no success, false if user has canceled
 */
Co::Task<bool>
StringEditDialog(ModalDock &dock,
		 std::string_view prompt,
		 std::string &value,
		 TextInputDialogOptions options={}) noexcept;
