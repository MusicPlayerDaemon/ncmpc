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

#include "screen_artist.hxx"
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

class SongListPage final : public FileListPage {
	Page *const parent;

	const enum mpd_tag_type artist_tag, album_tag;

	TagFilter filter;

public:
	SongListPage(ScreenManager &_screen, Page *_parent,
		     enum mpd_tag_type _artist_tag,
		     enum mpd_tag_type _album_tag,
		     WINDOW *_w, Size size) noexcept
		:FileListPage(_screen, _w, size,
			      options.list_format.c_str()),
		 parent(_parent),
		 artist_tag(_artist_tag), album_tag(_album_tag) {}

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
	TagListPage artist_list_page;
	TagListPage album_list_page;
	SongListPage song_list_page;

public:
	ArtistBrowserPage(ScreenManager &_screen, WINDOW *_w,
			  Size size)
		:ProxyPage(_w),
		 artist_list_page(_screen, nullptr,
				  MPD_TAG_ARTIST,
				  nullptr,
				  _w, size),
		 album_list_page(_screen, this,
				 MPD_TAG_ALBUM,
				 _("All tracks"),
				 _w, size),
		 song_list_page(_screen, this,
				artist_list_page.GetTag(),
				album_list_page.GetTag(),
				_w, size) {}

private:
	void OpenArtistList(struct mpdclient &c);
	void OpenAlbumList(struct mpdclient &c, std::string _artist);
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
ArtistBrowserPage::OpenArtistList(struct mpdclient &c)
{
	artist_list_page.SetFilter(TagFilter{}, _("All artists"));

	SetCurrentPage(c, &artist_list_page);
}

void
ArtistBrowserPage::OpenAlbumList(struct mpdclient &c, std::string _artist)
{
	char buffer[64];
	const char *title;

	if (_artist.empty()) {
		title = _("Albums");
	} else {
		snprintf(buffer, sizeof(buffer), _("Albums of artist: %s"),
			 Utf8ToLocale(_artist.c_str()).c_str());
		title = buffer;
	}

	album_list_page.SetFilter(TagFilter{{artist_list_page.GetTag(), std::move(_artist)}},
				  title);

	SetCurrentPage(c, &album_list_page);
}

void
ArtistBrowserPage::OpenSongList(struct mpdclient &c, TagFilter &&filter)
{
	song_list_page.SetFilter(std::move(filter));
	SetCurrentPage(c, &song_list_page);
}

static std::unique_ptr<Page>
screen_artist_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return std::make_unique<ArtistBrowserPage>(_screen, w, size);
}

const char *
SongListPage::GetTitle(char *str, size_t size) const noexcept
{
	const char *artist = FindTag(filter, artist_tag);
	if (artist == nullptr)
		artist = "?";

	const char *const album = FindTag(filter, album_tag);

	if (album == nullptr)
		snprintf(str, size,
			 _("All tracks of artist: %s"),
			 Utf8ToLocale(artist).c_str());
	else if (*album != '\0')
		snprintf(str, size, "%s: %s - %s",
			 _("Album"),
			 Utf8ToLocale(artist).c_str(),
			 Utf8ToLocale(album).c_str());
	else
		snprintf(str, size,
			 _("Tracks of no album of artist: %s"),
			 Utf8ToLocale(artist).c_str());

	return str;
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

	if (GetCurrentPage() == nullptr)
		OpenArtistList(c);
}

void
ArtistBrowserPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	artist_list_page.AddPendingEvents(events);
	album_list_page.AddPendingEvents(events);
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
		if (GetCurrentPage() == &artist_list_page) {
			const char *artist = artist_list_page.GetSelectedValue();
			if (artist != nullptr) {
				OpenAlbumList(c, artist);
				return true;
			}
		} else if (GetCurrentPage() == &album_list_page) {
			auto filter = album_list_page.MakeCursorFilter();
			if (!filter.empty())
				OpenSongList(c, std::move(filter));
		}

		break;

	case Command::GO_ROOT_DIRECTORY:
		if (GetCurrentPage() != &artist_list_page) {
			OpenArtistList(c);
			return true;
		}

		break;

	case Command::GO_PARENT_DIRECTORY:
		if (GetCurrentPage() == &album_list_page) {
			OpenArtistList(c);
			return true;
		} else if (GetCurrentPage() == &song_list_page) {
			OpenAlbumList(c, FindTag(song_list_page.GetFilter(),
						 artist_list_page.GetTag()));
			return true;
		}

		break;

	default:
		break;
	}

	return false;
}

const PageMeta screen_artist = {
	"artist",
	N_("Artist"),
	Command::SCREEN_ARTIST,
	screen_artist_init,
};
