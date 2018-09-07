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
#include "ArtistListPage.hxx"
#include "AlbumListPage.hxx"
#include "screen_interface.hxx"
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
#include "options.hxx"
#include "util/NulledString.hxx"

#include <vector>
#include <string>
#include <algorithm>

#include <assert.h>
#include <string.h>

class SongListPage final : public FileListPage {
	std::string artist;

	/**
	 * The current album filter.  If IsNulled() is true, then the
	 * album filter is not used (i.e. all songs from all albums
	 * are displayed).
	 */
	std::string album;

public:
	SongListPage(ScreenManager &_screen, WINDOW *_w, Size size)
		:FileListPage(_screen, _w, size,
			      options.list_format.c_str()) {}

	template<typename A>
	void SetArtist(A &&_artist) {
		artist = std::forward<A>(_artist);
		AddPendingEvents(~0u);
	}

	const std::string &GetArtist() {
		return artist;
	}

	template<typename A>
	void SetAlbum(A &&_album) {
		album = std::forward<A>(_album);
		AddPendingEvents(~0u);
	}

	const std::string &GetAlbum() {
		return album;
	}

	void LoadSongList(struct mpdclient &c);

	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

void
SongListPage::Update(struct mpdclient &c, unsigned events)
{
	if (events & MPD_IDLE_DATABASE) {
		LoadSongList(c);
	}
}

class ArtistBrowserPage final : public ProxyPage {
	ArtistListPage artist_list_page;
	AlbumListPage album_list_page;
	SongListPage song_list_page;

public:
	ArtistBrowserPage(ScreenManager &_screen, WINDOW *_w,
			  Size size)
		:ProxyPage(_w),
		 artist_list_page(_screen, _w, size),
		 album_list_page(_screen, _w, size),
		 song_list_page(_screen, _w, size) {}

private:
	void OpenArtistList(struct mpdclient &c);
	void OpenAlbumList(struct mpdclient &c, std::string _artist);
	void OpenSongList(struct mpdclient &c, std::string _artist,
			  std::string _album);

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Update(struct mpdclient &c, unsigned events) override;
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
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, artist.c_str());
		if (!IsNulled(album))
			mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
						      MPD_TAG_ALBUM, album.c_str());
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
	SetCurrentPage(c, &artist_list_page);
}

void
ArtistBrowserPage::OpenAlbumList(struct mpdclient &c, std::string _artist)
{
	album_list_page.SetArtist(std::move(_artist));
	SetCurrentPage(c, &album_list_page);
}

void
ArtistBrowserPage::OpenSongList(struct mpdclient &c, std::string _artist,
				std::string _album)
{
	song_list_page.SetArtist(std::move(_artist));
	song_list_page.SetAlbum(std::move(_album));
	SetCurrentPage(c, &song_list_page);
}

static Page *
screen_artist_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return new ArtistBrowserPage(_screen, w, size);
}

const char *
SongListPage::GetTitle(char *str, size_t size) const
{
	const Utf8ToLocale artist_locale(artist.c_str());

	if (IsNulled(album))
		snprintf(str, size,
			 _("All tracks of artist: %s"),
			 artist_locale.c_str());
	else if (!album.empty()) {
		const Utf8ToLocale album_locale(album.c_str());
		snprintf(str, size, _("Album: %s - %s"),
			 artist_locale.c_str(), album_locale.c_str());
	} else
		snprintf(str, size,
			 _("Tracks of no album of artist: %s"),
			 artist_locale.c_str());

	return str;
}

bool
SongListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch(cmd) {
	case Command::PLAY:
		if (lw.selected == 0) {
			/* handle ".." */
			screen.OnCommand(c, Command::GO_PARENT_DIRECTORY);
			return true;
		}

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
ArtistBrowserPage::OnOpen(struct mpdclient &c)
{
	ProxyPage::OnOpen(c);

	if (GetCurrentPage() == nullptr)
		SetCurrentPage(c, &artist_list_page);
}

void
ArtistBrowserPage::Update(struct mpdclient &c, unsigned events)
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
			const char *album = album_list_page.GetSelectedValue();
			if (album != nullptr)
				OpenSongList(c, album_list_page.GetArtist(),
					     album);
			else if (album_list_page.IsShowAll())
				OpenSongList(c, album_list_page.GetArtist(),
					     MakeNulledString());
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
			OpenAlbumList(c, song_list_page.GetArtist());
			return true;
		}

		break;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_artist = {
	"artist",
	screen_artist_init,
};
