/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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

#include "LibraryPage.hxx"
#include "TagListPage.hxx"
#include "PageMeta.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "FileListPage.hxx"
#include "Command.hxx"
#include "screen.hxx"
#include "ProxyPage.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "Options.hxx"

#include <vector>
#include <string>
#include <algorithm>

#include <assert.h>
#include <string.h>

gcc_const
static const char *
GetTagPlural(enum mpd_tag_type tag) noexcept
{
	switch (tag) {
	case MPD_TAG_ARTIST:
		return _("Artists");

	case MPD_TAG_ALBUM:
		return _("Albums");

	default:
		return mpd_tag_name(tag);
	}
}

static const char *
MakePageTitle(char *buffer, size_t size, const char *prefix,
	      const TagFilter &filter)
{
	if (filter.empty())
		return prefix;

	snprintf(buffer, size, "%s: %s", prefix,
		 Utf8ToLocale(ToString(filter).c_str()).c_str());
	return buffer;
}

class SongListPage final : public FileListPage {
	Page *const parent;

	TagFilter filter;

public:
	SongListPage(ScreenManager &_screen, Page *_parent,
		     WINDOW *_w, Size size) noexcept
		:FileListPage(_screen, _w, size,
			      options.list_format.c_str()),
		 parent(_parent) {}

	const auto &GetFilter() const noexcept {
		return filter;
	}

	template<typename F>
	void SetFilter(F &&_filter) noexcept {
		filter = std::forward<F>(_filter);
		AddPendingEvents(~0u);
	}

	void LoadSongList(struct mpdclient &c);

	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const noexcept override;
};

void
SongListPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (events & MPD_IDLE_DATABASE) {
		LoadSongList(c);
	}
}

class ArtistBrowserPage final : public ProxyPage {
	std::list<TagListPage> tag_list_pages;
	std::list<TagListPage>::iterator current_tag_list_page;

	SongListPage song_list_page;

public:
	ArtistBrowserPage(ScreenManager &_screen, WINDOW *_w,
			  Size size)
		:ProxyPage(_w),
		 song_list_page(_screen, this,
				_w, size) {

		bool first = true;
		for (const auto &tag : options.library_page_tags) {
			tag_list_pages.emplace_back(_screen,
						    first ? nullptr : this,
						    tag,
						    first ? nullptr : _("All"),
						    _w, size);
			first = false;
		}
	}

private:
	void OpenTagPage(struct mpdclient &c,
			 TagFilter &&filter) noexcept;
	void OpenSongList(struct mpdclient &c, TagFilter &&filter);

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
};

void
SongListPage::LoadSongList(struct mpdclient &c)
{
	auto *connection = c.GetConnection();

	delete filelist;

	filelist = new FileList();
	/* add a dummy entry for ".." */
	filelist->emplace_back(nullptr);

	if (connection != nullptr) {
		mpd_search_db_songs(connection, true);
		AddConstraints(connection, filter);
		mpd_search_commit(connection);

		filelist->Receive(*connection);

		c.FinishCommand();
	}

	/* fix highlights */
	screen_browser_sync_highlights(filelist, &c.playlist);
	lw.SetLength(filelist->size());
}

void
ArtistBrowserPage::OpenTagPage(struct mpdclient &c,
			       TagFilter &&filter) noexcept
{
	auto page = current_tag_list_page;

	page->SetFilter(std::move(filter));

	char buffer[64];
	page->SetTitle(MakePageTitle(buffer, sizeof(buffer),
				     _("Albums"),
				     page->GetFilter()));

	SetCurrentPage(c, &*page);
}

void
ArtistBrowserPage::OpenSongList(struct mpdclient &c, TagFilter &&filter)
{
	song_list_page.SetFilter(std::move(filter));
	SetCurrentPage(c, &song_list_page);
}

static std::unique_ptr<Page>
InitLibraryPage(ScreenManager &_screen, WINDOW *w, Size size)
{
	return std::make_unique<ArtistBrowserPage>(_screen, w, size);
}

const char *
SongListPage::GetTitle(char *str, size_t size) const noexcept
{
	return MakePageTitle(str, size, _("Songs"), filter);
}

bool
SongListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch(cmd) {
	case Command::PLAY:
		if (lw.selected == 0 && parent != nullptr)
			/* handle ".." */
			return parent->OnCommand(c, Command::GO_PARENT_DIRECTORY);

		break;

		/* continue and update... */
	case Command::SCREEN_UPDATE:
		LoadSongList(c);
		return false;

	default:
		break;
	}

	return FileListPage::OnCommand(c, cmd);
}

void
ArtistBrowserPage::OnOpen(struct mpdclient &c) noexcept
{
	ProxyPage::OnOpen(c);

	if (GetCurrentPage() == nullptr) {
		current_tag_list_page = tag_list_pages.begin();
		assert(current_tag_list_page != tag_list_pages.end());

		current_tag_list_page->SetTitle(GetTagPlural(current_tag_list_page->GetTag()));
		SetCurrentPage(c, &*current_tag_list_page);
	}
}

void
ArtistBrowserPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	for (auto &i : tag_list_pages)
		i.AddPendingEvents(events);
	song_list_page.AddPendingEvents(events);

	ProxyPage::Update(c, events);
}

bool
ArtistBrowserPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ProxyPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
	case Command::PLAY:
		if (current_tag_list_page != tag_list_pages.end()) {
			auto filter = current_tag_list_page->MakeCursorFilter();
			if (filter.empty())
				break;

			++current_tag_list_page;

			if (current_tag_list_page != tag_list_pages.end())
				OpenTagPage(c,  std::move(filter));
			else
				OpenSongList(c, std::move(filter));
			return true;
		}

		break;

	case Command::GO_ROOT_DIRECTORY:
		if (GetCurrentPage() != &tag_list_pages.front()) {
			current_tag_list_page = tag_list_pages.begin();
			SetCurrentPage(c, &*current_tag_list_page);
			return true;
		}

		break;

	case Command::GO_PARENT_DIRECTORY:
		if (current_tag_list_page != tag_list_pages.begin()) {
			--current_tag_list_page;
			SetCurrentPage(c, &*current_tag_list_page);
			return true;
		}

		break;

	default:
		break;
	}

	return false;
}

const PageMeta library_page = {
	"library",
	N_("Library"),
	Command::LIBRARY_PAGE,
	InitLibraryPage,
};
