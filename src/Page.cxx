// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Page.hxx"
#include "Window.hxx"

bool
Page::PaintStatusBarOverride(Window) const noexcept
{
	return false;
}
