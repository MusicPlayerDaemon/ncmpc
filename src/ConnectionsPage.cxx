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

#ifdef HAVE_AVAHI
#include "lib/avahi/Client.hxx"
#include "lib/avahi/ErrorHandler.hxx"
#include "lib/avahi/Explorer.hxx"
#include "lib/avahi/ExplorerListener.hxx"
#include "net/InetAddress.hxx"
#include "util/Exception.hxx"
#include "util/FNVHash.hxx"
#include "util/SpanCast.hxx"
#endif // HAVE_AVAHI

#include <fmt/format.h>

#include <cassert>
#include <vector>

using std::string_view_literals::operator""sv;

class ConnectionsPage final
	: public ListPage, ListText, ListRenderer
#ifdef HAVE_AVAHI
	, Avahi::ErrorHandler, Avahi::ServiceExplorerListener
#endif // HAVE_AVAHI
{
	ScreenManager &screen;

#ifdef HAVE_AVAHI
	Avahi::Client avahi_client{screen.GetEventLoop(), *this};
	Avahi::ServiceExplorer avahi_explorer{
		avahi_client,
		*this,
		AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		"_mpd._tcp", nullptr,
		*this,
	};
#endif // HAVE_AVAHI

	struct Item {
		MPD::SharedSettings settings;
		std::string name;
		bool active = false;

#ifdef HAVE_AVAHI
		std::string avahi_key;
#endif

		explicit Item(MPD::SharedSettings &&_settings) noexcept
			:settings(std::move(_settings)),
			 name(MPD::GetName(*settings)) {}

		explicit Item(std::string &&_name) noexcept
			:name(std::move(_name)) {}

		bool IsError() const noexcept {
			return !settings;
		}

		Style GetStyle() const noexcept {
			if (IsError())
				return Style::LIST_ALERT;
			else if (active)
				return Style::LIST_BOLD;
			else
				return Style::LIST;
		}

#ifdef HAVE_AVAHI
		[[gnu::pure]]
		auto GetHash() const noexcept {
			return FNV1aHash64(AsBytes(name));
		}
#endif // HAVE_AVAHI
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

private:
#ifdef HAVE_AVAHI
	/* virtual methods from class Avahi::ErrorHandler */
	bool OnAvahiError(std::exception_ptr e) noexcept override;

	/* virtual methods from class Avahi::ServiceExplorerListener */
	void OnAvahiNewObject(const std::string &key,
			      const char *host_name,
			      const InetAddress &address,
			      AvahiStringList *txt,
			      Avahi::ObjectFlags flags) noexcept override;
	void OnAvahiRemoveObject(const std::string &key) noexcept override;
#endif // HAVE_AVAHI
};

inline void
ConnectionsPage::Activate(struct mpdclient &c, unsigned i)
{
	const auto &item = items[i];
	if (item.IsError() || item.active)
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

	row_color(window, item.GetStyle(), selected);
	window.String(item.name);
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

#ifdef HAVE_AVAHI

bool
ConnectionsPage::OnAvahiError(std::exception_ptr e) noexcept
{
	if (items.back().IsError())
		items.pop_back();

	items.emplace_back(fmt::format("Zeroconf error: {}"sv, GetFullMessage(std::move(e))));

	lw.SetLength(items.size());
	SchedulePaint();

	return true;
}

void
ConnectionsPage::OnAvahiNewObject(const std::string &key,
				  const char *host_name,
				  const InetAddress &address,
				  [[maybe_unused]] AvahiStringList *txt,
				  [[maybe_unused]] Avahi::ObjectFlags flags) noexcept
{
	char buffer[64];
	const char *address_host = address.Format(buffer);
	if (address_host == nullptr)
		return;

	const auto hash = lw.GetCursorHash(items);

	auto pos = items.end();
	if (items.back().IsError())
		--pos;

	auto i = items.emplace(pos, MPD::NewSettings(address_host, address.GetPort(), 0, nullptr, nullptr));
	i->avahi_key = key;
	i->name = fmt::format("{} (Zeroconf {})"sv, host_name, address_host);

	lw.SetLength(items.size());
	lw.SetCursorHash(items, hash);
	SchedulePaint();
}

void
ConnectionsPage::OnAvahiRemoveObject(const std::string &key) noexcept
{
	const auto i = std::find_if(items.begin(), items.end(), [&key](const auto &item){
		return key == item.avahi_key;
	});
	if (i == items.end())
		return;

	const auto hash = lw.GetCursorHash(items);

	items.erase(i);

	lw.SetLength(items.size());
	lw.SetCursorHash(items, hash);
	SchedulePaint();
}

#endif // HAVE_AVAHI


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
