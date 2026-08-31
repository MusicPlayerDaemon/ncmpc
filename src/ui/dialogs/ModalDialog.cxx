// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ModalDialog.hxx"
#include "ModalDock.hxx"
#include "ui/Window.hxx"

#include <cassert>

ModalDialog::~ModalDialog() noexcept
{
	assert(!visible);
}

void
ModalDialog::OnLeave([[maybe_unused]] Window window) noexcept
{
}

void
ModalDialog::Show() noexcept
{
	assert(!visible);

	dock.ShowModalDialog(*this);
	visible = true;
}

void
ModalDialog::Hide() noexcept
{
	if (!visible)
		return;

	visible = false;
	dock.HideModalDialog(*this);
}

void
ModalDialog::InvokeCancel() noexcept
{
	assert(visible);

	visible = false;
	OnCancel();
}

void
ModalDialog::Cancel() noexcept
{
	assert(visible);

	dock.CancelModalDialog(*this);
}

void
ModalDialog::OnResize([[maybe_unused]] Window window,
		      [[maybe_unused]] Size size) noexcept
{
}
