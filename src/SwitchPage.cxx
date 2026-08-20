// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "SwitchPage.hxx"
#include "AllPages.hxx"
#include "PageMeta.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "config.h"
#include "i18n.h"
#include "screen.hxx"
#include "page/ListPage.hxx"
#include "ui/ListRenderer.hxx"
#include "ui/ListText.hxx"
#include "ui/TextListRenderer.hxx"
#include "ui/paint.hxx"
#include "util/LocaleString.hxx"

#include <iterator>

#include <assert.h>

class SwitchPage final : public ListPage, ListText {
	ScreenManager &screen;

	TextListRenderer list_renderer{*this};

public:
	SwitchPage(ScreenManager &_screen, const Window _window) noexcept
		:ListPage(_screen, _window), screen(_screen)
	{
		lw.SetLength(GetPageCount());
	}

public:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;

	/* virtual methods from class Page */
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	std::string_view GetTitle(std::span<char>) const noexcept override {
		return _("Page Switcher");
	}
};

std::string_view
SwitchPage::GetListItemText(std::span<char>, unsigned i) const noexcept
{
	const auto &page = *all_pages[i];
	return my_gettext(page.title);
}

void
SwitchPage::Paint() const noexcept
{
	lw.Paint(list_renderer);
}

bool
SwitchPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	if (!lw.IsCursorVisible())
		/* start searching at the beginning of the page (not
		   where the invisible cursor just happens to be),
		   unless the cursor is still visible from the last
		   search */
		lw.SetCursorFromOrigin(0);

	switch (cmd) {
	case Command::PLAY:
		screen.Switch(*all_pages[lw.GetCursorIndex()], c);
		return true;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		CoStart(screen.find_support.Find(lw, *this, cmd));
		return true;

	case Command::LIST_JUMP:
		CoStart(screen.find_support.Jump(lw, *this, list_renderer));
		return true;

	default:
		break;
	}

	return false;
}

#ifdef HAVE_GETMOUSE

bool
SwitchPage::OnMouse(struct mpdclient &c, Point p, mmask_t bstate)
{
	if (ListPage::OnMouse(c, p, bstate))
		return true;

	if (bstate & BUTTON1_CLICKED) {
		const unsigned old_selected = lw.GetCursorIndex();
		lw.SetCursorFromOrigin(p.y);
		const unsigned selected = lw.GetCursorIndex();

		if (selected != old_selected) {
			SchedulePaint();
			return true;
		}

		screen.Switch(*all_pages[lw.GetCursorIndex()], c);
		return true;
	} else if (bstate & BUTTON1_DOUBLE_CLICKED) {
		lw.SetCursorFromOrigin(p.y);
		screen.Switch(*all_pages[lw.GetCursorIndex()], c);
		return true;
	}

	return false;
}

#endif // HAVE_GETMOUSE

static std::unique_ptr<Page>
InitSwitchPage(ScreenManager &screen, const Window window)
{
	return std::make_unique<SwitchPage>(screen, window);
}

const PageMeta switch_page = {
	"switcher",
	N_("Page Switcher"),
	Command::SCREEN_SWITCH,
	InitSwitchPage,
};
