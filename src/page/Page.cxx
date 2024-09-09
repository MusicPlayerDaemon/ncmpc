// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Page.hxx"
#include "Container.hxx"
#include "ui/Window.hxx"

void
Page::SchedulePaint() noexcept
{
	parent.SchedulePaint(*this);
}

bool
Page::PaintStatusBarOverride(Window) const noexcept
{
	return false;
}
