// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "config.h"
#include "FileListPage.hxx"
#include "FileBrowserPage.hxx"
#include "SongPage.hxx"
#include "EditPlaylistPage.hxx"
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
#include "SongRowPaint.hxx"
#include "time_format.hxx"
#include "util/UriUtil.hxx"

#include <mpd/client.h>

#include <string.h>

using std::string_view_literals::operator""sv;

#define BUFSIZE 1024

#ifndef NCMPC_MINI
static constexpr unsigned HIGHLIGHT = 0x01;
#endif

FileListPage::FileListPage(ScreenManager &_screen, Window _window,
			   Size size,
			   const char *_song_format) noexcept
	:ListPage(_window, size),
	 screen(_screen),
	 song_format(_song_format)
{
}

FileListPage::~FileListPage() noexcept = default;

#ifndef NCMPC_MINI

/* sync highlight flags with playlist */
void
screen_browser_sync_highlights(FileList &fl, const MpdQueue &playlist) noexcept
{
	for (unsigned i = 0; i < fl.size(); ++i) {
		auto &entry = fl[i];
		const auto *entity = entry.entity;

		if (entity != nullptr && mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const auto *song = mpd_entity_get_song(entity);

			if (playlist.ContainsUri(mpd_song_get_uri(song)))
				entry.flags |= HIGHLIGHT;
			else
				entry.flags &= ~HIGHLIGHT;
		}
	}
}

#endif

std::string_view
FileListPage::GetListItemText(std::span<char> buffer,
			      unsigned idx) const noexcept
{
	assert(filelist != nullptr);
	assert(idx < filelist->size());

	const auto &entry = (*filelist)[idx];
	const auto *entity = entry.entity;

	if( entity == nullptr )
		return ".."sv;

	if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const auto *dir = mpd_entity_get_directory(entity);
		const char *name = GetUriFilename(mpd_directory_get_path(dir));
		return utf8_to_locale(name, buffer);
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
		const auto *song = mpd_entity_get_song(entity);

		return strfsong(buffer, options.list_format.c_str(), *song);
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
		const auto *playlist = mpd_entity_get_playlist(entity);
		const char *name = GetUriFilename(mpd_playlist_get_path(playlist));
		return utf8_to_locale(name, buffer);
	}

	return "Error: Unknown entry!"sv;
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
				     Utf8ToLocaleZ{name}.c_str());

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
	assert(song != nullptr);

	int id;

#ifndef NCMPC_MINI
	if (!(entry->flags & HIGHLIGHT))
		id = -1;
	else
#endif
		id = c->playlist.FindIdByUri(mpd_song_get_uri(song));

	if (id < 0) {
		id = mpd_run_add_id(connection, mpd_song_get_uri(song));
		if (id < 0) {
			c->HandleError();
			return false;
		}

#ifndef NCMPC_MINI
		entry->flags |= HIGHLIGHT;
#endif

		char buf[BUFSIZE];
		screen_status_fmt(_("Adding '{}' to queue"),
				  strfsong(buf, options.list_format.c_str(), *song));
	}

	if (!mpd_run_play_id(connection, id)) {
		c->HandleError();
		return false;
	}

	return true;
}

FileListEntry *
FileListPage::GetSelectedEntry() const noexcept
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
FileListPage::GetSelectedEntity() const noexcept
{
	const auto *entry = GetSelectedEntry();

	return entry != nullptr
		? entry->entity
		: nullptr;
}

const struct mpd_song *
FileListPage::GetSelectedSong() const noexcept
{
	const auto *entity = GetSelectedEntity();

	return entity != nullptr &&
		mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG
		? mpd_entity_get_song(entity)
		: nullptr;
}

FileListEntry *
FileListPage::GetIndex(unsigned i) const noexcept
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
browser_select_entry(struct mpdclient &c, FileListEntry &entry,
		     [[maybe_unused]] bool toggle) noexcept
{
	assert(entry.entity != nullptr);

	if (mpd_entity_get_type(entry.entity) == MPD_ENTITY_TYPE_PLAYLIST)
		return load_playlist(&c, mpd_entity_get_playlist(entry.entity));

	if (mpd_entity_get_type(entry.entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const auto *dir = mpd_entity_get_directory(entry.entity);

		if (!mpdclient_cmd_add_path(c, mpd_directory_get_path(dir)))
			return false;

		screen_status_fmt(_("Adding '{}' to queue"),
				  (std::string_view)Utf8ToLocale{mpd_directory_get_path(dir)});
		return true;
	}

	if (mpd_entity_get_type(entry.entity) != MPD_ENTITY_TYPE_SONG)
		return false;

#ifndef NCMPC_MINI
	if (!toggle || (entry.flags & HIGHLIGHT) == 0)
#endif
	{
		const auto *song = mpd_entity_get_song(entry.entity);

#ifndef NCMPC_MINI
		entry.flags |= HIGHLIGHT;
#endif

		if (c.RunAdd(*song)) {
			char buf[BUFSIZE];
			screen_status_fmt(_("Adding '{}' to queue"),
					  strfsong(buf, options.list_format.c_str(), *song));
		}
#ifndef NCMPC_MINI
	} else {
		/* remove song from playlist */
		const auto *song = mpd_entity_get_song(entry.entity);
		int idx;

		entry.flags &= ~HIGHLIGHT;

		while ((idx = c.playlist.FindByUri(mpd_song_get_uri(song))) >= 0)
			c.RunDelete(idx);
#endif
	}

	return true;
}

inline bool
FileListPage::HandleSelect(struct mpdclient &c) noexcept
{
	bool success = false;

	const auto range = lw.GetRange();
	for (const unsigned i : range) {
		auto *entry = GetIndex(i);
		if (entry != nullptr && entry->entity != nullptr)
			success = browser_select_entry(c, *entry, true);
	}

	SetDirty();

	return range.end_index == range.start_index + 1 && success;
}

inline bool
FileListPage::HandleAdd(struct mpdclient &c) noexcept
{
	bool success = false;

	const auto range = lw.GetRange();
	for (const unsigned i : range) {
		auto *entry = GetIndex(i);
		if (entry != nullptr && entry->entity != nullptr)
			success = browser_select_entry(c, *entry, false) ||
				success;
	}

	return range.end_index == range.start_index + 1 && success;
}

#ifdef ENABLE_PLAYLIST_EDITOR

inline bool
FileListPage::HandleEdit(struct mpdclient &c) noexcept
{
	const auto *entity = GetSelectedEntity();
	if (entity == nullptr ||
	    mpd_entity_get_type(entity) != MPD_ENTITY_TYPE_PLAYLIST)
		return false;

	const auto *playlist = mpd_entity_get_playlist(entity);
	assert(playlist != nullptr);

	EditPlaylist(screen, c, mpd_playlist_get_path(playlist));
	return true;
}

#endif

inline void
FileListPage::HandleSelectAll(struct mpdclient &c) noexcept
{
	if (filelist == nullptr)
		return;

	for (unsigned i = 0; i < filelist->size(); ++i) {
		auto &entry = (*filelist)[i];

		if (entry.entity != nullptr)
			browser_select_entry(c, entry, false);
	}

	SetDirty();
}

#ifdef HAVE_GETMOUSE

bool
FileListPage::OnMouse(struct mpdclient &c, Point p,
		      mmask_t bstate)
{
	unsigned prev_selected = lw.GetCursorIndex();

	if (ListPage::OnMouse(c, p, bstate))
		return true;

	lw.SetCursorFromOrigin(p.y);

	if (bstate & (BUTTON1_CLICKED|BUTTON1_DOUBLE_CLICKED)) {
		if ((bstate & BUTTON1_DOUBLE_CLICKED) ||
		    prev_selected == lw.GetCursorIndex())
			HandleEnter(c);
	} else if (bstate & (BUTTON3_CLICKED|BUTTON3_DOUBLE_CLICKED)) {
		if ((bstate & BUTTON3_DOUBLE_CLICKED) ||
		    prev_selected == lw.GetCursorIndex())
			HandleSelect(c);
	}

	SetDirty();

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
		if (const auto *song = GetSelectedSong()) {
			screen_song_switch(screen, c, *song);
			return true;
		} else
			return false;

#endif

#ifdef ENABLE_LYRICS_SCREEN
	case Command::SCREEN_LYRICS:
		if (const auto *song = GetSelectedSong()) {
			screen_lyrics_switch(screen, c, *song, false);
			return true;
		} else
			return false;

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

#ifdef ENABLE_PLAYLIST_EDITOR
	case Command::EDIT:
		if (HandleEdit(c))
			return true;
		break;
#endif

	case Command::SELECT_ALL:
		HandleSelectAll(c);
		return true;

	case Command::LOCATE:
		if (const auto *song = GetSelectedSong()) {
			screen_file_goto_song(screen, c, *song);
			return true;
		} else
			return false;


	default:
		break;
	}

	return false;
}

void
screen_browser_paint_directory(const Window window, unsigned width,
			       bool selected, std::string_view name) noexcept
{
	row_color(window, Style::DIRECTORY, selected);

	window.Char('[');
	window.String(name);
	window.Char(']');

	/* erase the unused space after the text */
	row_clear_to_eol(window, width, selected);
}

static void
screen_browser_paint_playlist(const Window window, unsigned width,
			      bool selected, std::string_view name) noexcept
{
	row_paint_text(window, width, Style::PLAYLIST, selected, name);
}

void
FileListPage::PaintListItem(const Window window, unsigned i,
			    unsigned y, unsigned width,
			    bool selected) const noexcept
{
	assert(filelist != nullptr);
	assert(i < filelist->size());

	const auto &entry = (*filelist)[i];
	const struct mpd_entity *entity = entry.entity;
	if (entity == nullptr) {
		screen_browser_paint_directory(window, width, selected, ".."sv);
		return;
	}

#ifndef NCMPC_MINI
	const bool highlight = (entry.flags & HIGHLIGHT) != 0;
#else
	constexpr bool highlight = false;
#endif

	switch (mpd_entity_get_type(entity)) {
	case MPD_ENTITY_TYPE_DIRECTORY: {
		const auto *directory = mpd_entity_get_directory(entity);
		const char *name = GetUriFilename(mpd_directory_get_path(directory));
		screen_browser_paint_directory(window, width, selected, Utf8ToLocale{name});
		break;
	}

	case MPD_ENTITY_TYPE_SONG:
		paint_song_row(window, y, width, selected, highlight,
			       *mpd_entity_get_song(entity), nullptr,
			       song_format);
		break;

	case MPD_ENTITY_TYPE_PLAYLIST: {
		const auto *playlist = mpd_entity_get_playlist(entity);
		const char *name = GetUriFilename(mpd_playlist_get_path(playlist));
		screen_browser_paint_playlist(window, width, selected, Utf8ToLocale{name});
		break;
	}

	default:
		row_paint_text(window, width,
			       highlight ? Style::LIST_BOLD : Style::LIST,
			       selected, "<unknown>");
	}
}

void
FileListPage::Paint() const noexcept
{
	lw.Paint(*this);
}

bool
FileListPage::PaintStatusBarOverride(const Window window) const noexcept
{
	if (!lw.HasRangeSelection())
		return false;

	window.MoveCursor({0, 0});
	window.ClearToEol();

	unsigned duration = 0;

	assert(filelist != nullptr);
	for (const unsigned i : lw.GetRange()) {
		assert(i < filelist->size());
		const auto &entry = (*filelist)[i];
		const auto *entity = entry.entity;

		if (entity != nullptr &&
		    mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
			duration += mpd_song_get_duration(mpd_entity_get_song(entity));
	}

	char duration_string[32];
	format_duration_short(duration_string, duration);
	const unsigned duration_width = strlen(duration_string);

	SelectStyle(window, Style::STATUS_TIME);
	window.String({0, (int)window.GetWidth() - (int)duration_width}, duration_string);

	window.RefreshNoOut();

	return true;
}
