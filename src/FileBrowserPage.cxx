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

#include "FileBrowserPage.hxx"
#include "screen_interface.hxx"
#include "FileListPage.hxx"
#include "screen_status.hxx"
#include "save_playlist.hxx"
#include "screen.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "filelist.hxx"
#include "screen_utils.hxx"
#include "screen_client.hxx"
#include "options.hxx"

#include <mpd/client.h>

#include <string>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

class FileBrowserPage final : public FileListPage {
	std::string current_path;

public:
	FileBrowserPage(ScreenManager &_screen, WINDOW *_w,
			Size size)
		:FileListPage(_screen, _w, size,
			      options.list_format) {}

	bool GotoSong(struct mpdclient &c, const struct mpd_song &song);

private:
	void Reload(struct mpdclient &c);

	/**
	 * Change to the specified absolute directory.
	 */
	bool ChangeDirectory(struct mpdclient &c, const char *new_path);

	/**
	 * Change to the parent directory of the current directory.
	 */
	bool ChangeToParent(struct mpdclient &c);

	/**
	 * Change to the directory referred by the specified
	 * #FileListEntry object.
	 */
	bool ChangeToEntry(struct mpdclient &c, const FileListEntry &entry);

	bool HandleEnter(struct mpdclient &c);
	void HandleSave(struct mpdclient &c);
	void HandleDelete(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

static void
screen_file_load_list(struct mpdclient *c, const char *current_path,
		      FileList *filelist)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return;

	mpd_send_list_meta(connection, current_path);
	filelist->Receive(*connection);

	if (c->FinishCommand())
		filelist->Sort();
}

void
FileBrowserPage::Reload(struct mpdclient &c)
{
	delete filelist;

	filelist = new FileList();
	if (!current_path.empty())
		/* add a dummy entry for ./.. */
		filelist->emplace_back(nullptr);

	screen_file_load_list(&c, current_path.c_str(), filelist);

	lw.SetLength(filelist->size());

	SetDirty();
}

bool
FileBrowserPage::ChangeDirectory(struct mpdclient &c, const char *new_path)
{
	current_path = new_path;

	Reload(c);

	screen_browser_sync_highlights(filelist, &c.playlist);

	lw.Reset();

	return filelist != nullptr;
}

bool
FileBrowserPage::ChangeToParent(struct mpdclient &c)
{
	char *parent = g_path_get_dirname(current_path.c_str());
	if (strcmp(parent, ".") == 0)
		parent[0] = '\0';

	auto old_path = std::move(current_path);

	bool success = ChangeDirectory(c, parent);
	g_free(parent);

	int idx = success
		? filelist->FindDirectory(old_path.c_str())
		: -1;

	if (success && idx >= 0) {
		/* set the cursor on the previous working directory */
		lw.SetCursor(idx);
		lw.Center(idx);
	}

	return success;
}

/**
 * Change to the directory referred by the specified #FileListEntry
 * object.
 */
bool
FileBrowserPage::ChangeToEntry(struct mpdclient &c, const FileListEntry &entry)
{
	if (entry.entity == nullptr)
		return ChangeToParent(c);
	else if (mpd_entity_get_type(entry.entity) == MPD_ENTITY_TYPE_DIRECTORY)
		return ChangeDirectory(c, mpd_directory_get_path(mpd_entity_get_directory(entry.entity)));
	else
		return false;
}

bool
FileBrowserPage::GotoSong(struct mpdclient &c, const struct mpd_song &song)
{
	const char *uri = mpd_song_get_uri(&song);
	if (strstr(uri, "//") != nullptr)
		/* an URL? */
		return false;

	/* determine the song's parent directory and go there */

	const char *slash = strrchr(uri, '/');
	char *allocated = nullptr;
	const char *parent = slash != nullptr
		? (allocated = g_strndup(uri, slash - uri))
		: "";

	bool ret = ChangeDirectory(c, parent);
	g_free(allocated);
	if (!ret)
		return false;

	/* select the specified song */

	int i = filelist->FindSong(song);
	if (i < 0)
		i = 0;

	lw.SetCursor(i);
	SetDirty();
	return true;
}

bool
FileBrowserPage::HandleEnter(struct mpdclient &c)
{
	const auto *entry = GetSelectedEntry();
	if (entry == nullptr)
		return false;

	return ChangeToEntry(c, *entry);
}

void
FileBrowserPage::HandleSave(struct mpdclient &c)
{
	const char *defaultname = nullptr;

	const auto range = lw.GetRange();
	if (range.start_index == range.end_index)
		return;

	for (const unsigned i : range) {
		auto &entry = (*filelist)[i];
		if (entry.entity) {
			struct mpd_entity *entity = entry.entity;
			if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
				const struct mpd_playlist *playlist =
					mpd_entity_get_playlist(entity);
				defaultname = mpd_playlist_get_path(playlist);
			}
		}
	}

	char *defaultname_utf8 = nullptr;
	if(defaultname)
		defaultname_utf8 = utf8_to_locale(defaultname);
	playlist_save(&c, nullptr, defaultname_utf8);
	g_free(defaultname_utf8);
}

void
FileBrowserPage::HandleDelete(struct mpdclient &c)
{
	auto *connection = c.GetConnection();

	if (connection == nullptr)
		return;

	const auto range = lw.GetRange();
	for (const unsigned i : range) {
		auto &entry = (*filelist)[i];
		if (entry.entity == nullptr)
			continue;

		const auto *entity = entry.entity;

		if (mpd_entity_get_type(entity) != MPD_ENTITY_TYPE_PLAYLIST) {
			/* translators: the "delete" command is only possible
			   for playlists; the user attempted to delete a song
			   or a directory or something else */
			screen_status_message(_("Deleting this item is not possible"));
			screen_bell();
			continue;
		}

		const auto *playlist = mpd_entity_get_playlist(entity);
		char *buf = g_strdup_printf(_("Delete playlist %s?"),
					    Utf8ToLocale(g_basename(mpd_playlist_get_path(playlist))).c_str());
		bool confirmed = screen_get_yesno(buf, false);
		g_free(buf);

		if (!confirmed) {
			/* translators: a dialog was aborted by the user */
			screen_status_message(_("Aborted"));
			return;
		}

		if (!mpd_run_rm(connection, mpd_playlist_get_path(playlist))) {
			c.HandleError();
			break;
		}

		c.events |= MPD_IDLE_STORED_PLAYLIST;

		/* translators: MPD deleted the playlist, as requested by the
		   user */
		screen_status_message(_("Playlist deleted"));
	}
}

static Page *
screen_file_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return new FileBrowserPage(_screen, w, size);
}

const char *
FileBrowserPage::GetTitle(char *str, size_t size) const
{
	const char *path = nullptr, *prev = nullptr, *slash = current_path.c_str();

	/* determine the last 2 parts of the path */
	while ((slash = strchr(slash, '/')) != nullptr) {
		path = prev;
		prev = ++slash;
	}

	if (path == nullptr)
		/* fall back to full path */
		path = current_path.c_str();

	g_snprintf(str, size, "%s: %s",
		   /* translators: caption of the browser screen */
		   _("Browse"), Utf8ToLocale(path).c_str());
	return str;
}

void
FileBrowserPage::Update(struct mpdclient &c, unsigned events)
{
	if (events & (MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST)) {
		/* the db has changed -> update the filelist */
		Reload(c);
	}

	if (events & (MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST
#ifndef NCMPC_MINI
		      | MPD_IDLE_QUEUE
#endif
		      )) {
		screen_browser_sync_highlights(filelist, &c.playlist);
		SetDirty();
	}
}

bool
FileBrowserPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	switch(cmd) {
	case CMD_PLAY:
		if (HandleEnter(c))
			return true;

		break;

	case CMD_GO_ROOT_DIRECTORY:
		ChangeDirectory(c, "");
		return true;
	case CMD_GO_PARENT_DIRECTORY:
		ChangeToParent(c);
		return true;

	case CMD_LOCATE:
		/* don't let browser_cmd() evaluate the locate command
		   - it's a no-op, and by the way, leads to a
		   segmentation fault in the current implementation */
		return false;

	case CMD_SCREEN_UPDATE:
		Reload(c);
		screen_browser_sync_highlights(filelist, &c.playlist);
		return false;

	default:
		break;
	}

	if (FileListPage::OnCommand(c, cmd))
		return true;

	if (!c.IsConnected())
		return false;

	switch(cmd) {
	case CMD_DELETE:
		HandleDelete(c);
		break;

	case CMD_SAVE_PLAYLIST:
		HandleSave(c);
		break;

	case CMD_DB_UPDATE:
		screen_database_update(&c, current_path.c_str());
		return true;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_browse = {
	"browse",
	screen_file_init,
};

bool
screen_file_goto_song(ScreenManager &_screen, struct mpdclient &c,
		      const struct mpd_song &song)
{
	auto pi = _screen.MakePage(screen_browse);
	auto &page = (FileBrowserPage &)*pi->second;
	if (!page.GotoSong(c, song))
		return false;

	/* finally, switch to the file screen */
	_screen.Switch(screen_browse, c);
	return true;
}
