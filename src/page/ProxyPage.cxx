// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ProxyPage.hxx"

#include <assert.h>

void
ProxyPage::SetCurrentPage(struct mpdclient &c, Page *new_page) noexcept
{
	if (current_page != nullptr && is_open)
		current_page->OnClose();

	current_page = new_page;

	if (current_page != nullptr && is_open) {
		current_page->OnOpen(c);
		current_page->Resize(GetLastSize());
		current_page->Update(c);
	}

	SchedulePaint();
}

void
ProxyPage::OnOpen(struct mpdclient &c) noexcept
{
	assert(!is_open);
	is_open = true;

	if (current_page != nullptr)
		current_page->OnOpen(c);
}

void
ProxyPage::OnClose() noexcept
{
	assert(is_open);
	is_open = false;

	if (current_page != nullptr)
		current_page->OnClose();
}

void
ProxyPage::OnResize(Size size) noexcept
{
	if (current_page != nullptr)
		current_page->Resize(size);
}

void
ProxyPage::Paint() const noexcept
{
	if (current_page != nullptr)
		current_page->Paint();
	else
		window.ClearToBottom();
}

void
ProxyPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (current_page != nullptr) {
		current_page->AddPendingEvents(events);
		current_page->Update(c);
	}
}

bool
ProxyPage::OnCommand(struct mpdclient &c, Command cmd)
{
	return current_page != nullptr &&
	       current_page->OnCommand(c, cmd);
}

#ifdef HAVE_GETMOUSE
bool
ProxyPage::OnMouse(struct mpdclient &c, Point p, mmask_t bstate)
{
	return current_page != nullptr &&
	       current_page->OnMouse(c, p, bstate);
}
#endif

std::string_view
ProxyPage::GetTitle(std::span<char> buffer) const noexcept
{
	return current_page != nullptr
		? current_page->GetTitle(buffer)
		: std::string_view{};
}

const struct mpd_song *
ProxyPage::GetSelectedSong() const noexcept
{
	return current_page != nullptr
		? current_page->GetSelectedSong()
		: nullptr;
}

void
ProxyPage::SchedulePaint(Page &page) noexcept
{
	if (&page == current_page)
		SchedulePaint();
}
