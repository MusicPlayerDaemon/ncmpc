// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "FileBrowserPage.hxx"
#include "PageMeta.hxx"
#include "FileListPage.hxx"
#include "save_playlist.hxx"
#include "screen.hxx"
#include "config.h" // IWYU pragma: keep
#include "i18n.h"
#include "charset.hxx"
#include "filelist.hxx"
#include "screen_client.hxx"
#include "Command.hxx"
#include "Options.hxx"
#include "dialogs/YesNoDialog.hxx"
#include "ui/Bell.hxx"
#include "lib/fmt/ToSpan.hxx"
#include "client/mpdclient.hxx"
#include "util/UriUtil.hxx"

#include <mpd/client.h>

#include <string>

#include <stdlib.h>
#include <string.h>

class FileBrowserPage final : public FileListPage {
	std::string current_path;

public:
	FileBrowserPage(ScreenManager &_screen, const Window _window) noexcept
		:FileListPage(_screen, _screen, _window,
			      options.list_format.c_str()) {}

	bool GotoSong(struct mpdclient &c, const struct mpd_song &song) noexcept;

private:
	void Reload(struct mpdclient &c) noexcept;

	/**
	 * Change to the specified absolute directory.
	 */
	bool ChangeDirectory(struct mpdclient &c,
			     std::string_view new_path) noexcept;

	/**
	 * Change to the parent directory of the current directory.
	 */
	bool ChangeToParent(struct mpdclient &c) noexcept;

	/**
	 * Change to the directory referred by the specified
	 * #FileListEntry object.
	 */
	bool ChangeToEntry(struct mpdclient &c,
			   const FileListEntry &entry) noexcept;

protected:
	bool HandleEnter(struct mpdclient &c) override;

private:
	void HandleSave(struct mpdclient &c) noexcept;

	[[nodiscard]]
	Co::InvokeTask HandleDelete(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;
};

static void
screen_file_load_list(struct mpdclient *c, const char *current_path,
		      FileList &filelist) noexcept
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return;

	mpd_send_list_meta(connection, current_path);
	filelist.Receive(*connection);

	if (c->FinishCommand())
		filelist.Sort();
}

void
FileBrowserPage::Reload(struct mpdclient &c) noexcept
{
	filelist = std::make_unique<FileList>();
	if (!current_path.empty())
		/* add a dummy entry for ./.. */
		filelist->emplace_back(nullptr);

	screen_file_load_list(&c, current_path.c_str(), *filelist);

	lw.SetLength(filelist->size());

	SchedulePaint();
}

bool
FileBrowserPage::ChangeDirectory(struct mpdclient &c,
				 std::string_view new_path) noexcept
{
	current_path = new_path;

	Reload(c);

	screen_browser_sync_highlights(*filelist, c.playlist);

	lw.Reset();

	return filelist != nullptr;
}

bool
FileBrowserPage::ChangeToParent(struct mpdclient &c) noexcept
{
	const auto old_path = std::move(current_path);

	bool success = ChangeDirectory(c, GetParentUri(old_path));

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
FileBrowserPage::ChangeToEntry(struct mpdclient &c,
			       const FileListEntry &entry) noexcept
{
	if (entry.entity == nullptr)
		return ChangeToParent(c);
	else if (mpd_entity_get_type(entry.entity) == MPD_ENTITY_TYPE_DIRECTORY)
		return ChangeDirectory(c, mpd_directory_get_path(mpd_entity_get_directory(entry.entity)));
	else
		return false;
}

bool
FileBrowserPage::GotoSong(struct mpdclient &c, const struct mpd_song &song) noexcept
{
	const char *uri = mpd_song_get_uri(&song);
	if (strstr(uri, "//") != nullptr)
		/* an URL? */
		return false;

	/* determine the song's parent directory and go there */

	if (!ChangeDirectory(c, GetParentUri(uri)))
		return false;

	/* select the specified song */

	int i = filelist->FindSong(song);
	if (i < 0)
		i = 0;

	lw.SetCursor(i);
	SchedulePaint();
	return true;
}

bool
FileBrowserPage::HandleEnter(struct mpdclient &c)
{
	const auto *entry = GetSelectedEntry();
	if (entry != nullptr && ChangeToEntry(c, *entry))
		return true;

	return FileListPage::HandleEnter(c);
}

void
FileBrowserPage::HandleSave(struct mpdclient &c) noexcept
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

	CoStart(playlist_save(screen, c,
			      defaultname != nullptr ? Utf8ToLocale{defaultname}.str() : std::string{}));
}

inline Co::InvokeTask
FileBrowserPage::HandleDelete(struct mpdclient &c)
{
	if (!c.IsConnected())
		co_return;

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
			Alert(_("Deleting this item is not possible"));
			Bell();
			continue;
		}

		const auto *playlist = mpd_entity_get_playlist(entity);

		const auto prompt = fmt::format(fmt::runtime(_("Delete playlist {}?")),
						(std::string_view)Utf8ToLocale{GetUriFilename(mpd_playlist_get_path(playlist))});

		if (co_await YesNoDialog{screen, prompt} != YesNoResult::YES)
			co_return;

		auto *connection = c.GetConnection();
		if (connection == nullptr)
			throw std::runtime_error{"Not connected"};

		if (!mpd_run_rm(connection, mpd_playlist_get_path(playlist))) {
			c.HandleError();
			break;
		}

		c.events |= MPD_IDLE_STORED_PLAYLIST;

		/* translators: MPD deleted the playlist, as requested by the
		   user */
		Alert(_("Playlist deleted"));
	}
}

static std::unique_ptr<Page>
screen_file_init(ScreenManager &_screen, const Window window) noexcept
{
	return std::make_unique<FileBrowserPage>(_screen, window);
}

std::string_view
FileBrowserPage::GetTitle(std::span<char> buffer) const noexcept
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

	return FmtTruncate(buffer, "{}: {}",
			   /* translators: caption of the browser screen */
			   _("Browse"), (std::string_view)Utf8ToLocale{path});
}

void
FileBrowserPage::Update(struct mpdclient &c, unsigned events) noexcept
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
		screen_browser_sync_highlights(*filelist, c.playlist);
		SchedulePaint();
	}
}

bool
FileBrowserPage::OnCommand(struct mpdclient &c, Command cmd)
{
	CoCancel();

	switch(cmd) {
	case Command::GO_ROOT_DIRECTORY:
		ChangeDirectory(c, {});
		return true;
	case Command::GO_PARENT_DIRECTORY:
		ChangeToParent(c);
		return true;

	case Command::SCREEN_UPDATE:
		Reload(c);
		screen_browser_sync_highlights(*filelist, c.playlist);
		return false;

	default:
		break;
	}

	if (FileListPage::OnCommand(c, cmd))
		return true;

	if (!c.IsReady())
		return false;

	switch(cmd) {
	case Command::DELETE:
		CoStart(HandleDelete(c));
		return true;

	case Command::SAVE_PLAYLIST:
		HandleSave(c);
		break;

	case Command::DB_UPDATE:
		screen_database_update(GetInterface(), c, current_path.c_str());
		return true;

	default:
		break;
	}

	return false;
}

const PageMeta screen_browse = {
	"browse",
	N_("Browse"),
	Command::SCREEN_FILE,
	screen_file_init,
};

bool
screen_file_goto_song(ScreenManager &_screen, struct mpdclient &c,
		      const struct mpd_song &song) noexcept
{
	auto pi = _screen.MakePage(screen_browse);
	auto &page = (FileBrowserPage &)*pi->second;
	if (!page.GotoSong(c, song))
		return false;

	/* finally, switch to the file screen */
	_screen.Switch(screen_browse, c);
	return true;
}
