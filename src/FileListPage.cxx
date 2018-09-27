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
#include "FileListPage.hxx"
#include "FileBrowserPage.hxx"
#include "SongPage.hxx"
#include "LyricsPage.hxx"
#include "Command.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "Options.hxx"
#include "charset.hxx"
#include "strfsong.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "Styles.hxx"
#include "paint.hxx"
#include "song_paint.hxx"
#include "util/UriUtil.hxx"

#include <mpd/client.h>

#include <string.h>

#define BUFSIZE 1024

#ifndef NCMPC_MINI
#define HIGHLIGHT  (0x01)
#endif

FileListPage::~FileListPage() noexcept
{
	delete filelist;
}

#ifndef NCMPC_MINI

/* sync highlight flags with playlist */
void
screen_browser_sync_highlights(FileList *fl, const MpdQueue *playlist)
{
	for (unsigned i = 0; i < fl->size(); ++i) {
		auto &entry = (*fl)[i];
		const auto *entity = entry.entity;

		if (entity != nullptr && mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const auto *song = mpd_entity_get_song(entity);

			if (playlist->ContainsUri(mpd_song_get_uri(song)))
				entry.flags |= HIGHLIGHT;
			else
				entry.flags &= ~HIGHLIGHT;
		}
	}
}

#endif

const char *
FileListPage::GetListItemText(char *buffer, size_t size,
			      unsigned idx) const noexcept
{
	assert(filelist != nullptr);
	assert(idx < filelist->size());

	const auto &entry = (*filelist)[idx];
	const auto *entity = entry.entity;

	if( entity == nullptr )
		return "..";

	if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const auto *dir = mpd_entity_get_directory(entity);
		const char *name = GetUriFilename(mpd_directory_get_path(dir));
		return utf8_to_locale(name, buffer, size);
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
		const auto *song = mpd_entity_get_song(entity);

		strfsong(buffer, size, options.list_format.c_str(), song);
		return buffer;
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
		const auto *playlist = mpd_entity_get_playlist(entity);
		const char *name = GetUriFilename(mpd_playlist_get_path(playlist));
		return utf8_to_locale(name, buffer, size);
	}

	return "Error: Unknown entry!";
}

static bool
load_playlist(struct mpdclient *c, const struct mpd_playlist *playlist)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	if (mpd_run_load(connection, mpd_playlist_get_path(playlist))) {
		const char *name = GetUriFilename(mpd_playlist_get_path(playlist));
		screen_status_printf(_("Loading playlist '%s'"),
				     Utf8ToLocale(name).c_str());

		c->events |= MPD_IDLE_QUEUE;
	} else
		c->HandleError();

	return true;
}

static bool
enqueue_and_play(struct mpdclient *c, FileListEntry *entry)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return false;

	const auto *song = mpd_entity_get_song(entry->entity);
	int id;

#ifndef NCMPC_MINI
	if (!(entry->flags & HIGHLIGHT))
		id = -1;
	else
#endif
		id = c->playlist.FindIdByUri(mpd_song_get_uri(song));

	if (id < 0) {
		char buf[BUFSIZE];

		id = mpd_run_add_id(connection, mpd_song_get_uri(song));
		if (id < 0) {
			c->HandleError();
			return false;
		}

#ifndef NCMPC_MINI
		entry->flags |= HIGHLIGHT;
#endif
		strfsong(buf, BUFSIZE, options.list_format.c_str(), song);
		screen_status_printf(_("Adding \'%s\' to queue"), buf);
	}

	if (!mpd_run_play_id(connection, id)) {
		c->HandleError();
		return false;
	}

	return true;
}

FileListEntry *
FileListPage::GetSelectedEntry() const
{
	const auto range = lw.GetRange();

	if (filelist == nullptr ||
	    range.empty() ||
	    range.end_index > range.start_index + 1 ||
	    range.start_index >= filelist->size())
		return nullptr;

	return &(*filelist)[range.start_index];
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
			screen_status_printf(_("Adding \'%s\' to queue"),
					     Utf8ToLocale(mpd_directory_get_path(dir)).c_str());
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

			strfsong(buf, BUFSIZE,
				 options.list_format.c_str(), song);
			screen_status_printf(_("Adding \'%s\' to queue"), buf);
		}
#ifndef NCMPC_MINI
	} else {
		/* remove song from playlist */
		const auto *song = mpd_entity_get_song(entry->entity);
		int idx;

		entry->flags &= ~HIGHLIGHT;

		while ((idx = c->playlist.FindByUri(mpd_song_get_uri(song))) >= 0)
			mpdclient_cmd_delete(c, idx);
#endif
	}

	return true;
}

bool
FileListPage::HandleSelect(struct mpdclient &c)
{
	bool success = false;

	const auto range = lw.GetRange();
	for (const unsigned i : range) {
		auto *entry = GetIndex(i);
		if (entry != nullptr && entry->entity != nullptr)
			success = browser_select_entry(&c, entry, true);
	}

	SetDirty();

	return range.end_index == range.start_index + 1 && success;
}

bool
FileListPage::HandleAdd(struct mpdclient &c)
{
	bool success = false;

	const auto range = lw.GetRange();
	for (const unsigned i : range) {
		auto *entry = GetIndex(i);
		if (entry != nullptr && entry->entity != nullptr)
			success = browser_select_entry(&c, entry, false) ||
				success;
	}

	return range.end_index == range.start_index + 1 && success;
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
FileListPage::OnMouse(struct mpdclient &c, Point p,
		      mmask_t bstate)
{
	unsigned prev_selected = lw.selected;

	if (ListPage::OnMouse(c, p, bstate))
		return true;

	lw.SetCursor(lw.start + p.y);

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
FileListPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (filelist == nullptr)
		return false;

	if (ListPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
#if defined(ENABLE_SONG_SCREEN) || defined(ENABLE_LYRICS_SCREEN)
		const struct mpd_song *song;
#endif

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, lw, cmd, *this);
		SetDirty();
		return true;
	case Command::LIST_JUMP:
		screen_jump(screen, lw, *this, *this);
		SetDirty();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case Command::SCREEN_SONG:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		screen_song_switch(screen, c, *song);
		return true;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	case Command::SCREEN_LYRICS:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		screen_lyrics_switch(screen, c, *song, false);
		return true;
#endif
	case Command::SCREEN_SWAP:
		screen.Swap(c, GetSelectedSong());
		return true;

	default:
		break;
	}

	if (!c.IsConnected())
		return false;

	switch (cmd) {
		const struct mpd_song *song;

	case Command::PLAY:
		HandleEnter(c);
		return true;

	case Command::SELECT:
		if (HandleSelect(c))
			lw.HandleCommand(Command::LIST_NEXT);
		SetDirty();
		return true;

	case Command::ADD:
		if (HandleAdd(c))
			lw.HandleCommand(Command::LIST_NEXT);
		SetDirty();
		return true;

	case Command::SELECT_ALL:
		HandleSelectAll(c);
		return true;

	case Command::LOCATE:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		screen_file_goto_song(screen, c, *song);
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
	row_color(w, Style::DIRECTORY, selected);

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
	row_paint_text(w, width, Style::PLAYLIST, selected, name);
}

void
FileListPage::PaintListItem(WINDOW *w, unsigned i,
			    unsigned y, unsigned width,
			    bool selected) const noexcept
{
	assert(filelist != nullptr);
	assert(i < filelist->size());

	const auto &entry = (*filelist)[i];
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
		const char *name;

	case MPD_ENTITY_TYPE_DIRECTORY:
		directory = mpd_entity_get_directory(entity);
		name = GetUriFilename(mpd_directory_get_path(directory));
		screen_browser_paint_directory(w, width, selected,
					       Utf8ToLocale(name).c_str());
		break;

	case MPD_ENTITY_TYPE_SONG:
		paint_song_row(w, y, width, selected, highlight,
			       mpd_entity_get_song(entity), nullptr,
			       song_format);
		break;

	case MPD_ENTITY_TYPE_PLAYLIST:
		playlist = mpd_entity_get_playlist(entity);
		name = GetUriFilename(mpd_playlist_get_path(playlist));
		screen_browser_paint_playlist(w, width, selected,
					      Utf8ToLocale(name).c_str());
		break;

	default:
		row_paint_text(w, width,
			       highlight ? Style::LIST_BOLD : Style::LIST,
			       selected, "<unknown>");
	}
}

void
FileListPage::Paint() const noexcept
{
	lw.Paint(*this);
}
