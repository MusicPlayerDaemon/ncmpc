// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

class ModalDialog;

/**
 * Interface for an entity which can host a #ModalDialog.
 */
class ModalDock {
public:
	virtual void ShowModalDialog(ModalDialog &m) noexcept = 0;
	virtual void HideModalDialog(ModalDialog &m) noexcept = 0;
	virtual void CancelModalDialog(ModalDialog &m) noexcept = 0;
};
