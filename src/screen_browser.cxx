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

#include "config.h"
#include "screen_browser.hxx"
#include "screen_file.hxx"
#include "screen_song.hxx"
#include "screen_lyrics.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "options.hxx"
#include "charset.hxx"
#include "strfsong.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "colors.hxx"
#include "paint.hxx"
#include "song_paint.hxx"

#include <mpd/client.h>

#include <string.h>

#define BUFSIZE 1024

#ifndef NCMPC_MINI
#define HIGHLIGHT  (0x01)
#endif

FileListPage::~FileListPage()
{
	delete filelist;
}

#ifndef NCMPC_MINI

/* sync highlight flags with playlist */
void
screen_browser_sync_highlights(FileList *fl,
			       const struct mpdclient_playlist *playlist)
{
	for (unsigned i = 0; i < fl->size(); ++i) {
		auto &entry = (*fl)[i];
		const auto *entity = entry.entity;

		if (entity != nullptr && mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const auto *song = mpd_entity_get_song(entity);

			if (playlist_get_index_from_same_song(playlist,
							      song) >= 0)
				entry.flags |= HIGHLIGHT;
			else
				entry.flags &= ~HIGHLIGHT;
		}
	}
}

#endif

/* list_window callback */
static const char *
browser_lw_callback(unsigned idx, void *data)
{
	const auto *fl = (const FileList *) data;
	static char buf[BUFSIZE];

	assert(fl != nullptr);
	assert(idx < fl->size());

	const auto &entry = (*fl)[idx];
	const auto *entity = entry.entity;

	if( entity == nullptr )
		return "..";

	if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const auto *dir = mpd_entity_get_directory(entity);
		char *directory = utf8_to_locale(g_basename(mpd_directory_get_path(dir)));
		g_strlcpy(buf, directory, sizeof(buf));
		g_free(directory);
		return buf;
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
		const auto *song = mpd_entity_get_song(entity);

		strfsong(buf, BUFSIZE, options.list_format, song);
		return buf;
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
		const auto *playlist = mpd_entity_get_playlist(entity);
		char *filename = utf8_to_locale(g_basename(mpd_playlist_get_path(playlist)));

		g_strlcpy(buf, filename, sizeof(buf));
		g_free(filename);
		return buf;
	}

	return "Error: Unknown entry!";
}

static bool
load_playlist(struct mpdclient *c, const struct mpd_playlist *playlist)
{
	auto *connection = mpdclient_get_connection(c);

	if (connection == nullptr)
		return false;

	if (mpd_run_load(connection, mpd_playlist_get_path(playlist))) {
		char *filename = utf8_to_locale(mpd_playlist_get_path(playlist));
		screen_status_printf(_("Loading playlist %s..."),
				     g_basename(filename));
		g_free(filename);

		c->events |= MPD_IDLE_QUEUE;
	} else
		mpdclient_handle_error(c);

	return true;
}

static bool
enqueue_and_play(struct mpdclient *c, FileListEntry *entry)
{
	auto *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return false;

	const auto *song = mpd_entity_get_song(entry->entity);
	int id;

#ifndef NCMPC_MINI
	if (!(entry->flags & HIGHLIGHT))
		id = -1;
	else
#endif
		id = playlist_get_id_from_same_song(&c->playlist, song);

	if (id < 0) {
		char buf[BUFSIZE];

		id = mpd_run_add_id(connection, mpd_song_get_uri(song));
		if (id < 0) {
			mpdclient_handle_error(c);
			return false;
		}

#ifndef NCMPC_MINI
		entry->flags |= HIGHLIGHT;
#endif
		strfsong(buf, BUFSIZE, options.list_format, song);
		screen_status_printf(_("Adding \'%s\' to queue"), buf);
	}

	if (!mpd_run_play_id(connection, id)) {
		mpdclient_handle_error(c);
		return false;
	}

	return true;
}

FileListEntry *
FileListPage::GetSelectedEntry() const
{
	ListWindowRange range;

	list_window_get_range(&lw, &range);

	if (filelist == nullptr ||
	    range.end <= range.start ||
	    range.end > range.start + 1 ||
	    range.start >= filelist->size())
		return nullptr;

	return &(*filelist)[range.start];
}

const struct mpd_entity *
FileListPage::GetSelectedEntity() const
{
	const auto *entry = GetSelectedEntry();

	return entry != nullptr
		? entry->entity
		: nullptr;
}

const struct mpd_song *
FileListPage::GetSelectedSong() const
{
	const auto *entity = GetSelectedEntity();

	return entity != nullptr &&
		mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG
		? mpd_entity_get_song(entity)
		: nullptr;
}

FileListEntry *
FileListPage::GetIndex(unsigned i) const
{
	if (filelist == nullptr || i >= filelist->size())
		return nullptr;

	return &(*filelist)[i];
}

bool
FileListPage::HandleEnter(struct mpdclient &c)
{
	auto *entry = GetSelectedEntry();
	if (entry == nullptr)
		return false;

	auto *entity = entry->entity;
	if (entity == nullptr)
		return false;

	if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST)
		return load_playlist(&c, mpd_entity_get_playlist(entity));
	else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
		return enqueue_and_play(&c, entry);
	return false;
}

static bool
browser_select_entry(struct mpdclient *c, FileListEntry *entry,
		     gcc_unused bool toggle)
{
	assert(entry != nullptr);
	assert(entry->entity != nullptr);

	if (mpd_entity_get_type(entry->entity) == MPD_ENTITY_TYPE_PLAYLIST)
		return load_playlist(c, mpd_entity_get_playlist(entry->entity));

	if (mpd_entity_get_type(entry->entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const auto *dir = mpd_entity_get_directory(entry->entity);

		if (mpdclient_cmd_add_path(c, mpd_directory_get_path(dir))) {
			char *tmp = utf8_to_locale(mpd_directory_get_path(dir));

			screen_status_printf(_("Adding \'%s\' to queue"), tmp);
			g_free(tmp);
		}

		return true;
	}

	if (mpd_entity_get_type(entry->entity) != MPD_ENTITY_TYPE_SONG)
		return false;

#ifndef NCMPC_MINI
	if (!toggle || (entry->flags & HIGHLIGHT) == 0)
#endif
	{
		const auto *song = mpd_entity_get_song(entry->entity);

#ifndef NCMPC_MINI
		entry->flags |= HIGHLIGHT;
#endif

		if (mpdclient_cmd_add(c, song)) {
			char buf[BUFSIZE];

			strfsong(buf, BUFSIZE, options.list_format, song);
			screen_status_printf(_("Adding \'%s\' to queue"), buf);
		}
#ifndef NCMPC_MINI
	} else {
		/* remove song from playlist */
		const auto *song = mpd_entity_get_song(entry->entity);
		int idx;

		entry->flags &= ~HIGHLIGHT;

		while ((idx = playlist_get_index_from_same_song(&c->playlist,
								song)) >= 0)
			mpdclient_cmd_delete(c, idx);
#endif
	}

	return true;
}

bool
FileListPage::HandleSelect(struct mpdclient &c)
{
	ListWindowRange range;
	bool success = false;

	list_window_get_range(&lw, &range);
	for (unsigned i = range.start; i < range.end; ++i) {
		auto *entry = GetIndex(i);
		if (entry != nullptr && entry->entity != nullptr)
			success = browser_select_entry(&c, entry, true);
	}

	SetDirty();

	return range.end == range.start + 1 && success;
}

bool
FileListPage::HandleAdd(struct mpdclient &c)
{
	ListWindowRange range;
	bool success = false;

	list_window_get_range(&lw, &range);
	for (unsigned i = range.start; i < range.end; ++i) {
		auto *entry = GetIndex(i);
		if (entry != nullptr && entry->entity != nullptr)
			success = browser_select_entry(&c, entry, false) ||
				success;
	}

	return range.end == range.start + 1 && success;
}

void
FileListPage::HandleSelectAll(struct mpdclient &c)
{
	if (filelist == nullptr)
		return;

	for (unsigned i = 0; i < filelist->size(); ++i) {
		auto &entry = (*filelist)[i];

		if (entry.entity != nullptr)
			browser_select_entry(&c, &entry, false);
	}

	SetDirty();
}

#ifdef HAVE_GETMOUSE

bool
FileListPage::OnMouse(struct mpdclient &c, gcc_unused int x, int row,
		      mmask_t bstate)
{
	unsigned prev_selected = lw.selected;

	if (list_window_mouse(&lw, bstate, row))
		return true;

	list_window_set_cursor(&lw, lw.start + row);

	if( bstate & BUTTON1_CLICKED ) {
		if (prev_selected == lw.selected)
			HandleEnter(c);
	} else if (bstate & BUTTON3_CLICKED) {
		if (prev_selected == lw.selected)
			HandleSelect(c);
	}

	return true;
}

#endif

bool
FileListPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (filelist == nullptr)
		return false;

	if (list_window_cmd(&lw, cmd)) {
		SetDirty();
		return true;
	}

	switch (cmd) {
#if defined(ENABLE_SONG_SCREEN) || defined(ENABLE_LYRICS_SCREEN)
		const struct mpd_song *song;
#endif

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(&lw, cmd, browser_lw_callback, filelist);
		SetDirty();
		return true;
	case CMD_LIST_JUMP:
		screen_jump(&lw,
			    browser_lw_callback, filelist,
			    PaintRow, this);
		SetDirty();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		screen_song_switch(&c, song);
		return true;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		screen_lyrics_switch(&c, song, false);
		return true;
#endif
	case CMD_SCREEN_SWAP:
		screen.Swap(&c, GetSelectedSong());
		return true;

	default:
		break;
	}

	if (!mpdclient_is_connected(&c))
		return false;

	switch (cmd) {
		const struct mpd_song *song;

	case CMD_PLAY:
		HandleEnter(c);
		return true;

	case CMD_SELECT:
		if (HandleSelect(c))
			list_window_cmd(&lw, CMD_LIST_NEXT);
		SetDirty();
		return true;

	case CMD_ADD:
		if (HandleAdd(c))
			list_window_cmd(&lw, CMD_LIST_NEXT);
		SetDirty();
		return true;

	case CMD_SELECT_ALL:
		HandleSelectAll(c);
		return true;

	case CMD_LOCATE:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		screen_file_goto_song(&c, song);
		return true;

	default:
		break;
	}

	return false;
}

void
screen_browser_paint_directory(WINDOW *w, unsigned width,
			       bool selected, const char *name)
{
	row_color(w, COLOR_DIRECTORY, selected);

	waddch(w, '[');
	waddstr(w, name);
	waddch(w, ']');

	/* erase the unused space after the text */
	row_clear_to_eol(w, width, selected);
}

static void
screen_browser_paint_playlist(WINDOW *w, unsigned width,
			      bool selected, const char *name)
{
	row_paint_text(w, width, COLOR_PLAYLIST, selected, name);
}

void
FileListPage::PaintRow(WINDOW *w, unsigned i,
		       unsigned y, unsigned width,
		       bool selected, const void *data)
{
	const auto &page = *(const FileListPage *) data;

	assert(page.filelist != nullptr);
	assert(i < page.filelist->size());

	const auto &entry = (*page.filelist)[i];
	const struct mpd_entity *entity = entry.entity;
	if (entity == nullptr) {
		screen_browser_paint_directory(w, width, selected, "..");
		return;
	}

#ifndef NCMPC_MINI
	const bool highlight = (entry.flags & HIGHLIGHT) != 0;
#else
	const bool highlight = false;
#endif

	switch (mpd_entity_get_type(entity)) {
		const struct mpd_directory *directory;
		const struct mpd_playlist *playlist;
		char *p;

	case MPD_ENTITY_TYPE_DIRECTORY:
		directory = mpd_entity_get_directory(entity);
		p = utf8_to_locale(g_basename(mpd_directory_get_path(directory)));
		screen_browser_paint_directory(w, width, selected, p);
		g_free(p);
		break;

	case MPD_ENTITY_TYPE_SONG:
		paint_song_row(w, y, width, selected, highlight,
			       mpd_entity_get_song(entity), nullptr,
			       page.song_format);
		break;

	case MPD_ENTITY_TYPE_PLAYLIST:
		playlist = mpd_entity_get_playlist(entity);
		p = utf8_to_locale(g_basename(mpd_playlist_get_path(playlist)));
		screen_browser_paint_playlist(w, width, selected, p);
		g_free(p);
		break;

	default:
		row_paint_text(w, width, highlight ? COLOR_LIST_BOLD : COLOR_LIST,
			       selected, "<unknown>");
	}
}

void
FileListPage::Paint() const
{
	list_window_paint2(&lw, PaintRow, this);
}
