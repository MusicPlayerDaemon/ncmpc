/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ProxyPage.hxx"

#include <assert.h>

void
ProxyPage::SetCurrentPage(struct mpdclient &c, Page *new_page)
{
	if (current_page != nullptr && is_open)
		current_page->OnClose();

	current_page = new_page;

	if (current_page != nullptr && is_open) {
		current_page->OnOpen(c);
		current_page->Resize(GetLastSize());
		current_page->Update(c);
		current_page->SetDirty(false);
	}

	SetDirty();
}

void
ProxyPage::OnOpen(struct mpdclient &c) noexcept
{
	assert(!is_open);
	is_open = true;

	if (current_page != nullptr)
		current_page->OnOpen(c);

	MoveDirty();
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

	MoveDirty();
}

void
ProxyPage::Paint() const noexcept
{
	if (current_page != nullptr)
		current_page->Paint();
	else
		wclrtobot(w);
}

void
ProxyPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (current_page != nullptr) {
		current_page->AddPendingEvents(events);
		current_page->Update(c);
		MoveDirty();
	}
}

bool
ProxyPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (current_page != nullptr) {
		bool result = current_page->OnCommand(c, cmd);
		MoveDirty();
		return result;
	} else
		return false;
}

#ifdef HAVE_GETMOUSE
bool
ProxyPage::OnMouse(struct mpdclient &c, Point p, mmask_t bstate)
{
	if (current_page != nullptr) {
		bool result = current_page->OnMouse(c, p, bstate);
		MoveDirty();
		return result;
	} else
		return false;
}
#endif

const char *
ProxyPage::GetTitle(char *s, size_t size) const noexcept
{
	return current_page != nullptr
		? current_page->GetTitle(s, size)
		: "";
}
