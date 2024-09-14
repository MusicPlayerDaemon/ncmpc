// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Page.hxx"
#include "Container.hxx"
#include "ui/Window.hxx"
#include "util/Exception.hxx"

void
Page::OnClose() noexcept
{
	// cancel any pending coroutine
	CoCancel();
}

void
Page::SchedulePaint() noexcept
{
	parent.SchedulePaint(*this);
}

void
Page::Alert(std::string message) noexcept
{
	parent.Alert(std::move(message));
}

void
Page::VFmtAlert(fmt::string_view format_str, fmt::format_args args) noexcept
{
	Alert(fmt::vformat(format_str, std::move(args)));
}

bool
Page::PaintStatusBarOverride(Window) const noexcept
{
	return false;
}

void
Page::OnCoComplete() noexcept
{
	SchedulePaint();
}

void
Page::CoStart(Co::InvokeTask _task) noexcept
{
	co_task = std::move(_task);
	co_task.Start(BIND_THIS_METHOD(_OnCoComplete));
}

inline void
Page::_OnCoComplete(std::exception_ptr error) noexcept
{
	if (error)
		parent.Alert(GetFullMessage(std::move(error)));
	else
		OnCoComplete();
}
