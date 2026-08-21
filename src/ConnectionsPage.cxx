// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "ConnectionsPage.hxx"
#include "Command.hxx"
#include "PageMeta.hxx"
#include "config.h"
#include "i18n.h"
#include "screen.hxx"
#include "client/mpdclient.hxx"
#include "client/Settings.hxx"
#include "client/SharedSettings.hxx"
#include "page/ListPage.hxx"
#include "ui/ListRenderer.hxx"
#include "ui/ListText.hxx"
#include "ui/ListRenderer.hxx"
#include "ui/paint.hxx"
#include "util/LocaleString.hxx"

#include <vector>

#include <assert.h>

class ConnectionsPage final : public ListPage, ListText, ListRenderer {
	ScreenManager &screen;

	struct Item {
		MPD::SharedSettings settings;
		const std::string name;
		bool active = false;

		explicit Item(MPD::SharedSettings &&_settings) noexcept
			:settings(std::move(_settings)),
			 name(MPD::GetName(*settings)) {}
	};

	std::vector<Item> items;

public:
	ConnectionsPage(ScreenManager &_screen, const Window _window) noexcept
		:ListPage(_screen, _window), screen(_screen)
	{
	}

private:
	void Activate(struct mpdclient &c, unsigned i);

public:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;

	/* virtual methods from class Page */
	virtual void OnOpen(struct mpdclient &c) noexcept override;
	void Paint() const noexcept override;
	void Update(struct mpdclient &, unsigned) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	std::string_view GetTitle(std::span<char>) const noexcept override {
		return _("Connections");
	}

	/* virtual methods from class ListRenderer */
	void PaintListItem(Window window, unsigned i, unsigned y, unsigned width,
			   bool selected) const noexcept override;
};

inline void
ConnectionsPage::Activate(struct mpdclient &c, unsigned i)
{
	const auto &item = items[i];
	if (item.active)
		return;

	c.Connect(MPD::SharedSettings{item.settings});
}


std::string_view
ConnectionsPage::GetListItemText(std::span<char>, unsigned i) const noexcept
{
	return items[i].name;
}

void
ConnectionsPage::OnOpen(struct mpdclient &c) noexcept
{
	if (items.empty()) {
		items.emplace_back(c.GetDefaultSettingsPtr());

		if (auto s = c.GetFallbackSettingsPtr())
			items.emplace_back(std::move(s));

		lw.SetLength(items.size());
	}
}

void
ConnectionsPage::PaintListItem(Window window, unsigned i, [[maybe_unused]] unsigned y, unsigned width,
			       bool selected) const noexcept
{
	const auto &item = items[i];
	const auto name = MPD::GetName(*item.settings);

	row_color(window, item.active ? Style::LIST_BOLD : Style::LIST, selected);
	window.String(name);
	row_clear_to_eol(window, width, selected);
}

void
ConnectionsPage::Paint() const noexcept
{
	lw.Paint(*this);
}

void
ConnectionsPage::Update(struct mpdclient &c, unsigned) noexcept
{
	const auto &active_settings = c.GetSettings();
	bool modified = false;

	for (auto &i : items) {
		const bool active = i.settings.get() == &active_settings;
		if (active != i.active) {
			i.active = active;
			modified = true;
		}
	}

	if (modified)
		SchedulePaint();
}

bool
ConnectionsPage::OnCommand(struct mpdclient &c, Command cmd)
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
		Activate(c, lw.GetCursorIndex());
		return true;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		CoStart(screen.find_support.Find(lw, *this, cmd));
		return true;

	case Command::LIST_JUMP:
		CoStart(screen.find_support.Jump(lw, *this, *this));
		return true;

	default:
		break;
	}

	return false;
}

#ifdef HAVE_GETMOUSE

bool
ConnectionsPage::OnMouse(struct mpdclient &c, Point p, mmask_t bstate)
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

		Activate(c, lw.GetCursorIndex());
		return true;
	} else if (bstate & BUTTON1_DOUBLE_CLICKED) {
		lw.SetCursorFromOrigin(p.y);
		Activate(c, lw.GetCursorIndex());
		return true;
	}

	return false;
}

#endif // HAVE_GETMOUSE

static std::unique_ptr<Page>
InitConnectionsPage(ScreenManager &screen, const Window window)
{
	return std::make_unique<ConnectionsPage>(screen, window);
}

const PageMeta connections_page = {
	"connections",
	N_("Connections"),
	Command::CONNECTIONS_PAGE,
	InitConnectionsPage,
};
