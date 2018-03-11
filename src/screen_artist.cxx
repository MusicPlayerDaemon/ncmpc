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
#include "screen_interface.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "screen_browser.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "options.hxx"

#include <vector>
#include <string>
#include <algorithm>

#include <assert.h>
#include <string.h>
#include <glib.h>

#define BUFSIZE 1024

class ArtistBrowserPage final : public FileListPage {
	enum class Mode {
		ARTISTS,
		ALBUMS,
		SONGS,
	} mode = Mode::ARTISTS;

	std::vector<std::string> artist_list, album_list;
	std::string artist;
	char *album  = nullptr;

public:
	ArtistBrowserPage(ScreenManager &_screen, WINDOW *_w,
			  Size size)
		:FileListPage(_screen, _w, size,
			      options.list_format) {}

	~ArtistBrowserPage() override {
		FreeState();
	}

private:
	void FreeLists();
	void FreeState();
	void LoadArtistList(struct mpdclient &c);
	void LoadAlbumList(struct mpdclient &c);
	void LoadSongList(struct mpdclient &c);
	void Reload(struct mpdclient &c);

	void OpenArtistList(struct mpdclient &c);
	void OpenArtistList(struct mpdclient &c, const char *selected_value);
	void OpenAlbumList(struct mpdclient &c, std::string _artist);
	void OpenAlbumList(struct mpdclient &c, std::string _artist,
			   const char *selected_value);
	void OpenSongList(struct mpdclient &c, std::string _artist,
			  char *_album);

	bool OnListCommand(struct mpdclient &c, command_t cmd);

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	void Update(struct mpdclient &c) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

gcc_pure
static int
string_array_find(const std::vector<std::string> &array, const char *value)
{
	for (size_t i = 0; i < array.size(); ++i)
		if (array[i] == value)
			return i;

	return -1;
}

gcc_pure
static bool
CompareUTF8(const std::string &a, const std::string &b)
{
	char *key1 = g_utf8_collate_key(a.c_str(), -1);
	char *key2 = g_utf8_collate_key(b.c_str(), -1);
	int n = strcmp(key1,key2);
	g_free(key1);
	g_free(key2);
	return n < 0;
}

/* list_window callback */
static const char *
screen_artist_lw_callback(unsigned idx, void *data)
{
	const auto &list = *(const std::vector<std::string> *)data;

	/*
	if (mode == Mode::ALBUMS) {
		if (idx == 0)
			return "..";
		else if (idx == list.size() + 1)
			return _("All tracks");

		--idx;
	}
	*/

	assert(idx < list.size());

	const char *str_utf8 = list[idx].c_str();
	char *str = utf8_to_locale(str_utf8);

	static char buf[BUFSIZE];
	g_strlcpy(buf, str, sizeof(buf));
	g_free(str);

	return buf;
}

/* list_window callback */
static const char *
AlbumListCallback(unsigned idx, void *data)
{
	const auto &list = *(const std::vector<std::string> *)data;

	if (idx == 0)
		return "..";
	else if (idx == list.size() + 1)
		return _("All tracks");

	--idx;

	return screen_artist_lw_callback(idx, data);
}

void
ArtistBrowserPage::FreeLists()
{
	artist_list.clear();
	album_list.clear();

	delete filelist;
	filelist = nullptr;
}

static void
recv_tag_values(struct mpd_connection *connection, enum mpd_tag_type tag,
		std::vector<std::string> &list)
{
	struct mpd_pair *pair;

	while ((pair = mpd_recv_pair_tag(connection, tag)) != nullptr) {
		list.emplace_back(pair->value);
		mpd_return_pair(connection, pair);
	}
}

void
ArtistBrowserPage::LoadArtistList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	assert(mode == Mode::ARTISTS);
	assert(album == nullptr);
	assert(artist_list.empty());
	assert(album_list.empty());
	assert(filelist == nullptr);

	artist_list.clear();

	if (connection != nullptr) {
		mpd_search_db_tags(connection, MPD_TAG_ARTIST);
		mpd_search_commit(connection);
		recv_tag_values(connection, MPD_TAG_ARTIST, artist_list);

		mpdclient_finish_command(&c);
	}

	/* sort list */
	std::sort(artist_list.begin(), artist_list.end(), CompareUTF8);
	lw.SetLength(artist_list.size());
}

void
ArtistBrowserPage::LoadAlbumList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	assert(mode == Mode::ALBUMS);
	assert(album == nullptr);
	assert(album_list.empty());
	assert(filelist == nullptr);

	if (connection != nullptr) {
		mpd_search_db_tags(connection, MPD_TAG_ALBUM);
		mpd_search_add_tag_constraint(connection,
					      MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, artist.c_str());
		mpd_search_commit(connection);

		recv_tag_values(connection, MPD_TAG_ALBUM, album_list);

		mpdclient_finish_command(&c);
	}

	/* sort list */
	std::sort(album_list.begin(), album_list.end(), CompareUTF8);
	lw.SetLength(album_list.size() + 2);
}

void
ArtistBrowserPage::LoadSongList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	assert(mode == Mode::SONGS);
	assert(filelist == nullptr);

	filelist = new FileList();
	/* add a dummy entry for ".." */
	filelist->emplace_back(nullptr);

	if (connection != nullptr) {
		mpd_search_db_songs(connection, true);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, artist.c_str());
		if (album != nullptr)
			mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
						      MPD_TAG_ALBUM, album);
		mpd_search_commit(connection);

		filelist->Receive(*connection);

		mpdclient_finish_command(&c);
	}

	/* fix highlights */
	screen_browser_sync_highlights(filelist, &c.playlist);
	lw.SetLength(filelist->size());
}

void
ArtistBrowserPage::FreeState()
{
	g_free(album);
	album = nullptr;

	FreeLists();
}

void
ArtistBrowserPage::OpenArtistList(struct mpdclient &c)
{
	FreeState();

	mode = Mode::ARTISTS;
	LoadArtistList(c);
}

void
ArtistBrowserPage::OpenArtistList(struct mpdclient &c,
				  const char *selected_value)
{
	OpenArtistList(c);

	lw.Reset();

	int idx = string_array_find(artist_list, selected_value);
	if (idx >= 0) {
		lw.SetCursor(idx);
		lw.Center(idx);
	}
}

void
ArtistBrowserPage::OpenAlbumList(struct mpdclient &c, std::string _artist)
{
	FreeState();

	mode = Mode::ALBUMS;
	artist = std::move(_artist);
	LoadAlbumList(c);
}

void
ArtistBrowserPage::OpenAlbumList(struct mpdclient &c, std::string _artist,
				 const char *selected_value)
{
	OpenAlbumList(c, std::move(_artist));

	lw.Reset();

	int idx = selected_value == nullptr
		? (int)album_list.size()
		: string_array_find(album_list, selected_value);
	if (idx >= 0) {
		++idx;
		lw.SetCursor(idx);
		lw.Center(idx);
	}
}

void
ArtistBrowserPage::OpenSongList(struct mpdclient &c,
				std::string _artist, char *_album)
{
	FreeState();

	mode = Mode::SONGS;
	artist = std::move(_artist);
	album = _album;
	LoadSongList(c);
}

void
ArtistBrowserPage::Reload(struct mpdclient &c)
{
	FreeLists();

	switch (mode) {
	case Mode::ARTISTS:
		LoadArtistList(c);
		break;

	case Mode::ALBUMS:
		LoadAlbumList(c);
		break;

	case Mode::SONGS:
		LoadSongList(c);
		break;
	}
}

static Page *
screen_artist_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return new ArtistBrowserPage(_screen, w, size);
}

void
ArtistBrowserPage::OnOpen(struct mpdclient &c)
{
	if (artist_list.empty())
		Reload(c);
}

/**
 * Paint one item in the artist list.
 */
static void
paint_artist_callback(WINDOW *w, unsigned i,
		      gcc_unused unsigned y, unsigned width,
		      bool selected, const void *data)
{
	const auto &list = *(const std::vector<std::string> *)data;
	char *p = utf8_to_locale(list[i].c_str());

	screen_browser_paint_directory(w, width, selected, p);
	g_free(p);
}

/**
 * Paint one item in the album list.  There are two virtual items
 * inserted: at the beginning, there's the special item ".." to go to
 * the parent directory, and at the end, there's the item "All tracks"
 * to view the tracks of all albums.
 */
static void
paint_album_callback(WINDOW *w, unsigned i,
		     gcc_unused unsigned y, unsigned width,
		     bool selected, const void *data)
{
	const auto &list = *(const std::vector<std::string> *)data;
	const char *p;
	char *q = nullptr;

	if (i == 0)
		p = "..";
	else if (i == list.size() + 1)
		p = _("All tracks");
	else
		p = q = utf8_to_locale(list[i - 1].c_str());

	screen_browser_paint_directory(w, width, selected, p);
	g_free(q);
}

void
ArtistBrowserPage::Paint() const
{
	switch (mode) {
	case Mode::ARTISTS:
		lw.Paint(paint_artist_callback, &artist_list);
		break;

	case Mode::ALBUMS:
		lw.Paint(paint_album_callback, &album_list);
		break;

	case Mode::SONGS:
		FileListPage::Paint();
		break;
	}
}

const char *
ArtistBrowserPage::GetTitle(char *str, size_t size) const
{
	switch(mode) {
		char *s1, *s2;

	case Mode::ARTISTS:
		g_snprintf(str, size, _("All artists"));
		break;

	case Mode::ALBUMS:
		s1 = utf8_to_locale(artist.c_str());
		g_snprintf(str, size, _("Albums of artist: %s"), s1);
		g_free(s1);
		break;

	case Mode::SONGS:
		s1 = utf8_to_locale(artist.c_str());

		if (album == nullptr)
			g_snprintf(str, size,
				   _("All tracks of artist: %s"), s1);
		else if (*album != 0) {
			s2 = utf8_to_locale(album);
			g_snprintf(str, size, _("Album: %s - %s"), s1, s2);
			g_free(s2);
		} else
			g_snprintf(str, size,
				   _("Tracks of no album of artist: %s"), s1);
		g_free(s1);
		break;
	}

	return str;
}

void
ArtistBrowserPage::Update(struct mpdclient &c)
{
	if (filelist == nullptr)
		return;

	if (c.events & MPD_IDLE_DATABASE)
		/* the db has changed -> update the list */
		Reload(c);

	if (c.events & (MPD_IDLE_DATABASE | MPD_IDLE_QUEUE))
		screen_browser_sync_highlights(filelist, &c.playlist);

	if (c.events & (MPD_IDLE_DATABASE
#ifndef NCMPC_MINI
			| MPD_IDLE_QUEUE
#endif
			))
		SetDirty();
}

/* add_query - Add all songs satisfying specified criteria.
   _artist is actually only used in the ALBUM case to distinguish albums with
   the same name from different artists. */
static void
add_query(struct mpdclient *c, enum mpd_tag_type table, const char *_filter,
	  const char *_artist)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);

	assert(_filter != nullptr);

	if (connection == nullptr)
		return;

	char *str = utf8_to_locale(_filter);
	if (table == MPD_TAG_ALBUM)
		screen_status_printf(_("Adding album %s..."), str);
	else
		screen_status_printf(_("Adding %s..."), str);
	g_free(str);

	mpd_search_db_songs(connection, true);
	mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
				      table, _filter);
	if (table == MPD_TAG_ALBUM)
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, _artist);
	mpd_search_commit(connection);

	auto *addlist = filelist_new_recv(connection);

	if (mpdclient_finish_command(c))
		mpdclient_filelist_add_all(c, addlist);

	delete addlist;
}

inline bool
ArtistBrowserPage::OnListCommand(struct mpdclient &c, command_t cmd)
{
	switch (mode) {
	case Mode::ARTISTS:
	case Mode::ALBUMS:
		if (lw.HandleCommand(cmd)) {
			SetDirty();
			return true;
		}

		return false;

	case Mode::SONGS:
		return FileListPage::OnCommand(c, cmd);
	}

	assert(0);
	return 0;
}

bool
ArtistBrowserPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	switch(cmd) {
		const char *selected;
		char *old;

	case CMD_PLAY:
		switch (mode) {
		case Mode::ARTISTS:
			if (lw.selected >= artist_list.size())
				return true;

			selected = artist_list[lw.selected].c_str();
			OpenAlbumList(c, g_strdup(selected));
			lw.Reset();

			SetDirty();
			return true;

		case Mode::ALBUMS:
			if (lw.selected == 0) {
				/* handle ".." */
				OpenArtistList(c, artist.c_str());
			} else if (lw.selected == album_list.size() + 1) {
				/* handle "show all" */
				OpenSongList(c, std::move(artist), nullptr);
				lw.Reset();
			} else {
				/* select album */
				selected = album_list[lw.selected - 1].c_str();
				OpenSongList(c, std::move(artist),
					     g_strdup(selected));
				lw.Reset();
			}

			SetDirty();
			return true;

		case Mode::SONGS:
			if (lw.selected == 0) {
				/* handle ".." */
				old = g_strdup(album);

				OpenAlbumList(c, std::move(artist), old);
				g_free(old);
				SetDirty();
				return true;
			}
			break;
		}
		break;

		/* FIXME? CMD_GO_* handling duplicates code from CMD_PLAY */

	case CMD_GO_PARENT_DIRECTORY:
		switch (mode) {
		case Mode::ARTISTS:
			break;

		case Mode::ALBUMS:
			OpenArtistList(c, artist.c_str());
			break;

		case Mode::SONGS:
			old = g_strdup(album);

			OpenAlbumList(c, std::move(artist), old);
			g_free(old);
			break;
		}

		SetDirty();
		break;

	case CMD_GO_ROOT_DIRECTORY:
		switch (mode) {
		case Mode::ARTISTS:
			break;

		case Mode::ALBUMS:
		case Mode::SONGS:
			OpenArtistList(c);
			lw.Reset();
			/* restore first list window state (pop while returning true) */
			/* XXX */
			break;
		}

		SetDirty();
		break;

	case CMD_SELECT:
	case CMD_ADD:
		switch(mode) {
		case Mode::ARTISTS:
			if (lw.selected >= artist_list.size())
				return true;

			for (const unsigned i : lw.GetRange()) {
				selected = artist_list[i].c_str();
				add_query(&c, MPD_TAG_ARTIST, selected, nullptr);
				cmd = CMD_LIST_NEXT; /* continue and select next item... */
			}
			break;

		case Mode::ALBUMS:
			for (const unsigned i : lw.GetRange()) {
				if(i == album_list.size() + 1)
					add_query(&c, MPD_TAG_ARTIST,
						  artist.c_str(), nullptr);
				else if (i > 0)
				{
					selected = album_list[lw.selected - 1].c_str();
					add_query(&c, MPD_TAG_ALBUM, selected,
						  artist.c_str());
					cmd = CMD_LIST_NEXT; /* continue and select next item... */
				}
			}
			break;

		case Mode::SONGS:
			/* handled by browser_cmd() */
			break;
		}
		break;

		/* continue and update... */
	case CMD_SCREEN_UPDATE:
		Reload(c);
		return false;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		switch (mode) {
		case Mode::ARTISTS:
			screen_find(screen, &lw, cmd,
				    screen_artist_lw_callback, &artist_list);
			SetDirty();
			return true;

		case Mode::ALBUMS:
			screen_find(screen, &lw, cmd,
				    AlbumListCallback, &album_list);
			SetDirty();
			return true;

		case Mode::SONGS:
			/* handled by browser_cmd() */
			break;
		}

		break;

	case CMD_LIST_JUMP:
		switch (mode) {
		case Mode::ARTISTS:
			screen_jump(screen, &lw,
				    screen_artist_lw_callback, &artist_list,
				    paint_artist_callback, &artist_list);
			SetDirty();
			return true;

		case Mode::ALBUMS:
			screen_jump(screen, &lw,
				    AlbumListCallback, &album_list,
				    paint_album_callback, &album_list);
			SetDirty();
			return true;

		case Mode::SONGS:
			/* handled by browser_cmd() */
			break;
		}

		break;

	default:
		break;
	}

	if (OnListCommand(c, cmd))
		return true;

	return false;
}

const struct screen_functions screen_artist = {
	"artist",
	screen_artist_init,
};
