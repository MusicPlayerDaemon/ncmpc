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
#include "screen.hxx"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "options.hxx"

#include <assert.h>
#include <string.h>
#include <glib.h>

#define BUFSIZE 1024

static char ALL_TRACKS[] = "";

class ArtistBrowserPage final : public FileListPage {
	enum class Mode {
		ARTISTS,
		ALBUMS,
		SONGS,
	} mode = Mode::ARTISTS;

	GPtrArray *artist_list = nullptr, *album_list = nullptr;
	char *artist = nullptr;
	char *album  = nullptr;

public:
	ArtistBrowserPage(WINDOW *_w, unsigned _cols, unsigned _rows)
		:FileListPage(_w, _cols, _rows,
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
	void OpenAlbumList(struct mpdclient &c, char *_artist);
	void OpenSongList(struct mpdclient &c, char *_artist, char *_album);

	bool OnListCommand(struct mpdclient &c, command_t cmd);

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	void Update(struct mpdclient &c) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

static gint
compare_utf8(gconstpointer s1, gconstpointer s2)
{
	const char *const*t1 = (const char *const*)s1, *const*t2 = (const char *const*)s2;

	char *key1 = g_utf8_collate_key(*t1,-1);
	char *key2 = g_utf8_collate_key(*t2,-1);
	int n = strcmp(key1,key2);
	g_free(key1);
	g_free(key2);
	return n;
}

/* list_window callback */
static const char *
screen_artist_lw_callback(unsigned idx, void *data)
{
	GPtrArray *list = (GPtrArray *)data;

	/*
	if (mode == Mode::ALBUMS) {
		if (idx == 0)
			return "..";
		else if (idx == list->len + 1)
			return _("All tracks");

		--idx;
	}
	*/

	assert(idx < list->len);

	const char *str_utf8 = (char *)g_ptr_array_index(list, idx);
	assert(str_utf8 != nullptr);

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
	GPtrArray *list = (GPtrArray *)data;

	if (idx == 0)
		return "..";
	else if (idx == list->len + 1)
		return _("All tracks");

	--idx;

	return screen_artist_lw_callback(idx, data);
}

static void
string_array_free(GPtrArray *array)
{
	for (unsigned i = 0; i < array->len; ++i) {
		char *value = (char *)g_ptr_array_index(array, i);
		g_free(value);
	}

	g_ptr_array_free(array, true);
}

void
ArtistBrowserPage::FreeLists()
{
	if (artist_list != nullptr) {
		string_array_free(artist_list);
		artist_list = nullptr;
	}

	if (album_list != nullptr) {
		string_array_free(album_list);
		album_list = nullptr;
	}

	delete filelist;
	filelist = nullptr;
}

static void
recv_tag_values(struct mpd_connection *connection, enum mpd_tag_type tag,
		GPtrArray *list)
{
	struct mpd_pair *pair;

	while ((pair = mpd_recv_pair_tag(connection, tag)) != nullptr) {
		g_ptr_array_add(list, g_strdup(pair->value));
		mpd_return_pair(connection, pair);
	}
}

void
ArtistBrowserPage::LoadArtistList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	assert(mode == Mode::ARTISTS);
	assert(artist == nullptr);
	assert(album == nullptr);
	assert(artist_list == nullptr);
	assert(album_list == nullptr);
	assert(filelist == nullptr);

	artist_list = g_ptr_array_new();

	if (connection != nullptr) {
		mpd_search_db_tags(connection, MPD_TAG_ARTIST);
		mpd_search_commit(connection);
		recv_tag_values(connection, MPD_TAG_ARTIST, artist_list);

		mpdclient_finish_command(&c);
	}

	/* sort list */
	g_ptr_array_sort(artist_list, compare_utf8);
	list_window_set_length(&lw, artist_list->len);
}

void
ArtistBrowserPage::LoadAlbumList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	assert(mode == Mode::ALBUMS);
	assert(artist != nullptr);
	assert(album == nullptr);
	assert(album_list == nullptr);
	assert(filelist == nullptr);

	album_list = g_ptr_array_new();

	if (connection != nullptr) {
		mpd_search_db_tags(connection, MPD_TAG_ALBUM);
		mpd_search_add_tag_constraint(connection,
					      MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, artist);
		mpd_search_commit(connection);

		recv_tag_values(connection, MPD_TAG_ALBUM, album_list);

		mpdclient_finish_command(&c);
	}

	/* sort list */
	g_ptr_array_sort(album_list, compare_utf8);

	list_window_set_length(&lw, album_list->len + 2);
}

void
ArtistBrowserPage::LoadSongList(struct mpdclient &c)
{
	struct mpd_connection *connection = mpdclient_get_connection(&c);

	assert(mode == Mode::SONGS);
	assert(artist != nullptr);
	assert(album != nullptr);
	assert(filelist == nullptr);

	filelist = new FileList();
	/* add a dummy entry for ".." */
	filelist->emplace_back(nullptr);

	if (connection != nullptr) {
		mpd_search_db_songs(connection, true);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, artist);
		if (album != ALL_TRACKS)
			mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
						      MPD_TAG_ALBUM, album);
		mpd_search_commit(connection);

		filelist->Receive(*connection);

		mpdclient_finish_command(&c);
	}

	/* fix highlights */
	screen_browser_sync_highlights(filelist, &c.playlist);
	list_window_set_length(&lw, filelist->size());
}

void
ArtistBrowserPage::FreeState()
{
	g_free(artist);
	if (album != ALL_TRACKS)
		g_free(album);
	artist = nullptr;
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
ArtistBrowserPage::OpenAlbumList(struct mpdclient &c, char *_artist)
{
	assert(_artist != nullptr);

	FreeState();

	mode = Mode::ALBUMS;
	artist = _artist;
	LoadAlbumList(c);
}

void
ArtistBrowserPage::OpenSongList(struct mpdclient &c,
				char *_artist, char *_album)
{
	assert(_artist != nullptr);
	assert(_album != nullptr);

	FreeState();

	mode = Mode::SONGS;
	artist = _artist;
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
screen_artist_init(WINDOW *w, unsigned cols, unsigned rows)
{
	return new ArtistBrowserPage(w, cols, rows);
}

void
ArtistBrowserPage::OnOpen(struct mpdclient &c)
{
	if (artist_list == nullptr && album_list == nullptr &&
	    filelist == nullptr)
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
	const GPtrArray *list = (const GPtrArray *)data;
	char *p = utf8_to_locale((const char *)g_ptr_array_index(list, i));

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
	const GPtrArray *list = (const GPtrArray *)data;
	const char *p;
	char *q = nullptr;

	if (i == 0)
		p = "..";
	else if (i == list->len + 1)
		p = _("All tracks");
	else
		p = q = utf8_to_locale((const char *)g_ptr_array_index(list, i - 1));

	screen_browser_paint_directory(w, width, selected, p);
	g_free(q);
}

void
ArtistBrowserPage::Paint() const
{
	if (filelist) {
		FileListPage::Paint();
	} else if (album_list != nullptr)
		list_window_paint2(&lw,
				   paint_album_callback, album_list);
	else if (artist_list != nullptr)
		list_window_paint2(&lw,
				   paint_artist_callback, artist_list);
	else {
		wmove(lw.w, 0, 0);
		wclrtobot(lw.w);
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
		s1 = utf8_to_locale(artist);
		g_snprintf(str, size, _("Albums of artist: %s"), s1);
		g_free(s1);
		break;

	case Mode::SONGS:
		s1 = utf8_to_locale(artist);

		if (album == ALL_TRACKS)
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
		if (list_window_cmd(&lw, cmd)) {
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

static int
string_array_find(GPtrArray *array, const char *value)
{
	guint i;

	for (i = 0; i < array->len; ++i)
		if (strcmp((const char*)g_ptr_array_index(array, i),
			   value) == 0)
			return i;

	return -1;
}

bool
ArtistBrowserPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	switch(cmd) {
		ListWindowRange range;
		const char *selected;
		char *old;
		char *old_ptr;
		int idx;

	case CMD_PLAY:
		switch (mode) {
		case Mode::ARTISTS:
			if (lw.selected >= artist_list->len)
				return true;

			selected = (const char *)g_ptr_array_index(artist_list,
								   lw.selected);
			OpenAlbumList(c, g_strdup(selected));
			list_window_reset(&lw);

			SetDirty();
			return true;

		case Mode::ALBUMS:
			if (lw.selected == 0) {
				/* handle ".." */
				old = g_strdup(artist);

				OpenArtistList(c);
				list_window_reset(&lw);
				/* restore previous list window state */
				idx = string_array_find(artist_list, old);
				g_free(old);

				if (idx >= 0) {
					list_window_set_cursor(&lw, idx);
					list_window_center(&lw, idx);
				}
			} else if (lw.selected == album_list->len + 1) {
				/* handle "show all" */
				OpenSongList(c, g_strdup(artist), ALL_TRACKS);
				list_window_reset(&lw);
			} else {
				/* select album */
				selected = (const char *)g_ptr_array_index(album_list,
									   lw.selected - 1);
				OpenSongList(c, g_strdup(artist), g_strdup(selected));
				list_window_reset(&lw);
			}

			SetDirty();
			return true;

		case Mode::SONGS:
			if (lw.selected == 0) {
				/* handle ".." */
				old = g_strdup(album);
				old_ptr = album;

				OpenAlbumList(c, g_strdup(artist));
				list_window_reset(&lw);
				/* restore previous list window state */
				idx = old_ptr == ALL_TRACKS
					? (int)album_list->len
					: string_array_find(album_list, old);
				g_free(old);

				if (idx >= 0) {
					++idx;
					list_window_set_cursor(&lw, idx);
					list_window_center(&lw, idx);
				}

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
			old = g_strdup(artist);

			OpenArtistList(c);
			list_window_reset(&lw);
			/* restore previous list window state */
			idx = string_array_find(artist_list, old);
			g_free(old);

			if (idx >= 0) {
				list_window_set_cursor(&lw, idx);
				list_window_center(&lw, idx);
			}
			break;

		case Mode::SONGS:
			old = g_strdup(album);
			old_ptr = album;

			OpenAlbumList(c, g_strdup(artist));
			list_window_reset(&lw);
			/* restore previous list window state */
			idx = old_ptr == ALL_TRACKS
				? (int)album_list->len
				: string_array_find(album_list, old);
			g_free(old);

			if (idx >= 0) {
				++idx;
				list_window_set_cursor(&lw, idx);
				list_window_center(&lw, idx);
			}
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
			list_window_reset(&lw);
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
			if (lw.selected >= artist_list->len)
				return true;

			list_window_get_range(&lw, &range);
			for (unsigned i = range.start; i < range.end; ++i) {
				selected = (const char *)g_ptr_array_index(artist_list, i);
				add_query(&c, MPD_TAG_ARTIST, selected, nullptr);
				cmd = CMD_LIST_NEXT; /* continue and select next item... */
			}
			break;

		case Mode::ALBUMS:
			list_window_get_range(&lw, &range);
			for (unsigned i = range.start; i < range.end; ++i) {
				if(i == album_list->len + 1)
					add_query(&c, MPD_TAG_ARTIST, artist, nullptr);
				else if (i > 0)
				{
					selected = (const char *)g_ptr_array_index(album_list,
										   lw.selected - 1);
					add_query(&c, MPD_TAG_ALBUM, selected, artist);
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
			screen_find(&lw, cmd,
				    screen_artist_lw_callback, artist_list);
			SetDirty();
			return true;

		case Mode::ALBUMS:
			screen_find(&lw, cmd,
				    AlbumListCallback, album_list);
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
			screen_jump(&lw,
				    screen_artist_lw_callback, artist_list,
				    paint_artist_callback, artist_list);
			SetDirty();
			return true;

		case Mode::ALBUMS:
			screen_jump(&lw,
				    AlbumListCallback, album_list,
				    paint_album_callback, album_list);
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
	.init = screen_artist_init,
};
