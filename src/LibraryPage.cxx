// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "LibraryPage.hxx"
#include "TagListPage.hxx"
#include "PageMeta.hxx"
#include "FileListPage.hxx"
#include "Command.hxx"
#include "ProxyPage.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "Options.hxx"

#include <list>
#include <string>
#include <vector>

#include <assert.h>
#include <string.h>

[[gnu::const]]
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

protected:
	/* virtual methods from class FileListPage */
	bool HandleEnter(struct mpdclient &c) override;

public:
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

class ArtistBrowserPage;

class LibraryTagListPage final : public TagListPage {
	ArtistBrowserPage &library_page;

public:
	LibraryTagListPage(ScreenManager &_screen,
			   ArtistBrowserPage &_library_page,
			   Page *_parent,
			   const enum mpd_tag_type _tag,
			   const char *_all_text,
			   WINDOW *_w, Size size) noexcept
		:TagListPage(_screen, _parent, _tag, _all_text, _w, size),
		 library_page(_library_page) {}

protected:
	bool HandleEnter(struct mpdclient &c) override;
};

class ArtistBrowserPage final : public ProxyPage {
	std::list<LibraryTagListPage> tag_list_pages;
	std::list<LibraryTagListPage>::iterator current_tag_list_page;

	SongListPage song_list_page;

public:
	ArtistBrowserPage(ScreenManager &_screen, WINDOW *_w,
			  Size size)
		:ProxyPage(_w),
		 song_list_page(_screen, this,
				_w, size) {

		bool first = true;
		for (const auto &tag : options.library_page_tags) {
			tag_list_pages.emplace_back(_screen, *this,
						    first ? nullptr : this,
						    tag,
						    first ? nullptr : _("All"),
						    _w, size);
			first = false;
		}
	}

	void EnterTag(struct mpdclient &c, TagFilter &&filter);

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

	filelist = std::make_unique<FileList>();
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
	screen_browser_sync_highlights(*filelist, c.playlist);
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
SongListPage::HandleEnter(struct mpdclient &c)
{
	if (lw.GetCursorIndex() == 0 && parent != nullptr)
		/* handle ".." */
		return parent->OnCommand(c, Command::GO_PARENT_DIRECTORY);

	return FileListPage::HandleEnter(c);
}

bool
SongListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch(cmd) {
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

inline void
ArtistBrowserPage::EnterTag(struct mpdclient &c, TagFilter &&filter)
{
	assert(current_tag_list_page != tag_list_pages.end());

	++current_tag_list_page;

	if (current_tag_list_page != tag_list_pages.end()) {
		while (true) {
			OpenTagPage(c,  std::move(filter));
			if (current_tag_list_page->HasMultipleValues())
				break;

			/* skip tags which have just
			   one value */
			filter = current_tag_list_page->GetFilter();
			++current_tag_list_page;
			if (current_tag_list_page == tag_list_pages.end()) {
				OpenSongList(c, std::move(filter));
				break;
			}
		}
	} else
		OpenSongList(c, std::move(filter));
}

bool
LibraryTagListPage::HandleEnter(struct mpdclient &c)
{
	auto new_filter = MakeCursorFilter();
	if (new_filter.empty())
		return TagListPage::HandleEnter(c);

	library_page.EnterTag(c, std::move(new_filter));
	return true;
}

bool
ArtistBrowserPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ProxyPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
	case Command::GO_ROOT_DIRECTORY:
		if (GetCurrentPage() != &tag_list_pages.front()) {
			current_tag_list_page = tag_list_pages.begin();
			SetCurrentPage(c, &*current_tag_list_page);
			return true;
		}

		break;

	case Command::GO_PARENT_DIRECTORY:
		while (current_tag_list_page != tag_list_pages.begin()) {
			--current_tag_list_page;
			SetCurrentPage(c, &*current_tag_list_page);
			if (current_tag_list_page->HasMultipleValues())
				return true;

			/* skip tags which have just one value */
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
