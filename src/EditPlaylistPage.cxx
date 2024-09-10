// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "EditPlaylistPage.hxx"
#include "PageMeta.hxx"
#include "FileListPage.hxx"
#include "filelist.hxx"
#include "SongPtr.hxx"
#include "config.h"
#include "i18n.h"
#include "Command.hxx"
#include "Options.hxx"
#include "mpdclient.hxx"
#include "screen.hxx"
#include "lib/fmt/ToSpan.hxx"

#include <mpd/client.h>

#include <string>

using std::string_view_literals::operator""sv;

static std::string next_playlist_name;

class EditPlaylistPage final : public FileListPage {
	std::string name;

	SongPtr selected_song;

public:
	EditPlaylistPage(ScreenManager &_screen, const Window _window, Size size) noexcept
		:FileListPage(_screen, _screen, _window, size,
			      options.list_format.c_str())
	{
	}

private:
	void Reload(struct mpdclient &c);

	void SaveSelection() noexcept;
	void RestoreSelection() noexcept;

	bool HandleDelete(struct mpdclient &c);
	bool HandleMoveUp(struct mpdclient &c);
	bool HandleMoveDown(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	std::string_view GetTitle(std::span<char> buffer) const noexcept override;
};

static bool
LoadPlaylist(struct mpdclient &c, const char *name, FileList &l)
{
	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return false;

	mpd_send_list_playlist_meta(connection, name);
	l.Receive(*connection);
	return c.FinishCommand();
}

static struct std::unique_ptr<FileList>
LoadPlaylist(struct mpdclient &c, const std::string &name)
{
	auto l = std::make_unique<FileList>();
	if (!name.empty())
		LoadPlaylist(c, name.c_str(), *l);

	return l;
}

void
EditPlaylistPage::Reload(struct mpdclient &c)
{
	filelist = LoadPlaylist(c, name.c_str());
	lw.SetLength(filelist->size());
	SchedulePaint();
}

void
EditPlaylistPage::SaveSelection() noexcept
{
	const auto *song = GetSelectedSong();
	selected_song.reset(song != nullptr
			    ? mpd_song_dup(song)
			    : nullptr);
}

void
EditPlaylistPage::RestoreSelection() noexcept
{
	if (!filelist || !selected_song)
		/* there was no selection */
		return;

	int i = filelist->FindSong(*selected_song);
	if (i >= 0)
		lw.SetCursor(i);

	SaveSelection();
}

static std::unique_ptr<Page>
edit_playlist_page_init(ScreenManager &_screen, const Window window, Size size)
{
	return std::make_unique<EditPlaylistPage>(_screen, window, size);
}

std::string_view
EditPlaylistPage::GetTitle(std::span<char> buffer) const noexcept
{
	if (name.empty())
		return _("Playlist");

	return FmtTruncate(buffer, "{}: {}"sv, _("Playlist"), name);
}

void
EditPlaylistPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	bool need_reload = events & MPD_IDLE_STORED_PLAYLIST;

	if (!next_playlist_name.empty()) {
		if (next_playlist_name != name) {
			lw.SetCursor(0);
			selected_song.reset();
			name = std::move(next_playlist_name);
			need_reload = true;
		}

		next_playlist_name.clear();
	}

	if (need_reload) {
		Reload(c);
		RestoreSelection();
	}
}

#ifdef HAVE_GETMOUSE
bool
EditPlaylistPage::OnMouse(struct mpdclient &c, Point p, mmask_t bstate)
{
	if (FileListPage::OnMouse(c, p, bstate))
		return true;

	if (bstate & BUTTON1_DOUBLE_CLICKED) {
		/* stop */

		auto *connection = c.GetConnection();
		if (connection != nullptr &&
		    !mpd_run_stop(connection))
			c.HandleError();
		return true;
	}

	const unsigned old_selected = lw.GetCursorIndex();
	lw.SetCursorFromOrigin(p.y);

	if (bstate & BUTTON3_CLICKED) {
		/* delete */
		if (lw.GetCursorIndex() == old_selected) {
			auto *connection = c.GetConnection();
			if (connection != nullptr &&
			    !mpd_run_playlist_delete(connection, name.c_str(),
						     lw.GetCursorIndex()))
				c.HandleError();
		}
	}

	SaveSelection();
	SchedulePaint();

	return true;
}
#endif

inline bool
EditPlaylistPage::HandleDelete(struct mpdclient &c)
{
	const auto range = lw.GetRange();
	if (range.empty())
		return true;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return true;

	mpd_command_list_begin(connection, false);

	for (unsigned i = range.end_index; i > range.start_index; --i)
		mpd_send_playlist_delete(connection, name.c_str(), i - 1);

	if (!mpd_command_list_end(connection) ||
	    !mpd_response_finish(connection))
		c.HandleError();

	lw.SetCursor(range.start_index);

	return true;
}

#if LIBMPDCLIENT_CHECK_VERSION(2,19,0)

inline bool
EditPlaylistPage::HandleMoveUp(struct mpdclient &c)
{
	const auto range = lw.GetRange();
	if (range.start_index == 0 || range.empty())
		return false;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return true;

	SaveSelection();

	if (mpd_run_playlist_move(connection, name.c_str(),
				  range.start_index - 1, range.end_index - 1))
		lw.SelectionMovedUp();
	else
		c.HandleError();

	return true;
}

inline bool
EditPlaylistPage::HandleMoveDown(struct mpdclient &c)
{
	const auto range = lw.GetRange();
	if (range.end_index >= lw.GetLength())
		return false;

	auto *connection = c.GetConnection();
	if (connection == nullptr)
		return true;

	SaveSelection();

	if (mpd_run_playlist_move(connection, name.c_str(),
				  range.end_index, range.start_index))
		lw.SelectionMovedDown();
	else
		c.HandleError();

	return true;
}

#endif

bool
EditPlaylistPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (FileListPage::OnCommand(c, cmd)) {
		SaveSelection();
		return true;
	}

	if (!c.IsReady())
		return false;

	switch (cmd) {
	case Command::DELETE:
		return HandleDelete(c);

	case Command::SAVE_PLAYLIST:
		//playlist_save(screen, c);
		// TODO
		return true;

	case Command::SHUFFLE:
		// TODO
		return true;

#if LIBMPDCLIENT_CHECK_VERSION(2,19,0)
	case Command::LIST_MOVE_UP:
		return HandleMoveUp(c);

	case Command::LIST_MOVE_DOWN:
		return HandleMoveDown(c);
#endif

	default:
		break;
	}

	return false;
}

const PageMeta edit_playlist_page = {
	"playlist_editor",
	N_("Playlist Editor"),
	Command::PLAYLIST_EDITOR_PAGE,
	edit_playlist_page_init,
};

void
EditPlaylist(ScreenManager &_screen, struct mpdclient &c,
	     const char *name) noexcept
{
	next_playlist_name = name;
	_screen.Switch(edit_playlist_page, c);
}
