// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "QueuePage.hxx"
#include "PageMeta.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"
#include "FileBrowserPage.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "save_playlist.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "Command.hxx"
#include "Options.hxx"
#include "mpdclient.hxx"
#include "strfsong.hxx"
#include "Completion.hxx"
#include "Styles.hxx"
#include "SongRowPaint.hxx"
#include "time_format.hxx"
#include "screen.hxx"
#include "screen_utils.hxx"
#include "SongPage.hxx"
#include "LyricsPage.hxx"
#include "db_completion.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "util/SPrintf.hxx"

#ifndef NCMPC_MINI
#include "hscroll.hxx"
#include "TableGlue.hxx"
#include "TableStructure.hxx"
#include "TableLayout.hxx"
#include "TablePaint.hxx"
#endif

#include <mpd/client.h>

#include <set>
#include <string>

#include <string.h>

#define MAX_SONG_LENGTH 512

class QueuePage final : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;

#ifndef NCMPC_MINI
	mutable class hscroll hscroll;

	TableLayout table_layout;
#endif

	CoarseTimerEvent hide_cursor_timer;

	MpdQueue *playlist = nullptr;
	int current_song_id = -1;
	int selected_song_id = -1;

	unsigned last_connection_id = 0;
	std::string connection_name;

	bool playing = false;

public:
	QueuePage(ScreenManager &_screen, Window _window,
		  Size size)
		:ListPage(_window, size),
		 screen(_screen),
#ifndef NCMPC_MINI
		 hscroll(screen.GetEventLoop(),
			 _window, options.scroll_sep),
		 table_layout(song_table_structure),
#endif
		 hide_cursor_timer(screen.GetEventLoop(),
				   BIND_THIS_METHOD(OnHideCursorTimer))
	{
#ifndef NCMPC_MINI
		table_layout.Calculate(size.width);
#endif
	}

private:
	[[gnu::pure]]
	const struct mpd_song *GetSelectedSong() const;

	void SaveSelection();
	void RestoreSelection();

	void Repaint() const {
		Paint();
		lw.Refresh();
	}

	void CenterPlayingItem(const struct mpd_status *status,
			       bool center_cursor);

	bool OnSongChange(const struct mpd_status *status);

	void OnHideCursorTimer() noexcept;

	void ScheduleHideCursor() {
		assert(options.hide_cursor > std::chrono::steady_clock::duration::zero());

		hide_cursor_timer.Schedule(options.hide_cursor);
	}

	/* virtual methods from class ListRenderer */
	void PaintListItem(Window window, unsigned i,
			   unsigned y, unsigned width,
			   bool selected) const noexcept override;

	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void OnClose() noexcept override;

	void OnResize(Size size) noexcept override {
		ListPage::OnResize(size);
#ifndef NCMPC_MINI
		table_layout.Calculate(size.width);
#endif
	}

	void Paint() const noexcept override;
	bool PaintStatusBarOverride(Window window) const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	std::string_view GetTitle(std::span<char> buffer) const noexcept override;
};

const struct mpd_song *
QueuePage::GetSelectedSong() const
{
	return lw.IsSingleCursor()
		? &(*playlist)[lw.GetCursorIndex()]
		: nullptr;
}

void
QueuePage::SaveSelection()
{
	selected_song_id = GetSelectedSong() != nullptr
		? (int)mpd_song_get_id(GetSelectedSong())
		: -1;
}

void
QueuePage::RestoreSelection()
{
	lw.SetLength(playlist->size());

	if (selected_song_id < 0)
		/* there was no selection */
		return;

	const struct mpd_song *song = GetSelectedSong();
	if (song != nullptr &&
	    mpd_song_get_id(song) == (unsigned)selected_song_id)
		/* selection is still valid */
		return;

	int pos = playlist->FindById(selected_song_id);
	if (pos >= 0)
		lw.SetCursor(pos);

	SaveSelection();
}

std::string_view 
QueuePage::GetListItemText(std::span<char> buffer,
			   unsigned idx) const noexcept
{
	assert(idx < playlist->size());

	const auto &song = (*playlist)[idx];
	return strfsong(buffer, options.list_format.c_str(), song);
}

void
QueuePage::CenterPlayingItem(const struct mpd_status *status,
			     bool center_cursor)
{
	if (status == nullptr ||
	    (mpd_status_get_state(status) != MPD_STATE_PLAY &&
	     mpd_status_get_state(status) != MPD_STATE_PAUSE))
		return;

	/* try to center the song that are playing */
	int idx = mpd_status_get_song_pos(status);
	if (idx < 0)
		return;

	lw.Center(idx);

	if (center_cursor) {
		lw.SetCursor(idx);
		return;
	}

	/* make sure the cursor is in the window */
	lw.FetchCursor();
}

[[gnu::pure]]
static int
get_current_song_id(const struct mpd_status *status)
{
	return status != nullptr &&
		(mpd_status_get_state(status) == MPD_STATE_PLAY ||
		 mpd_status_get_state(status) == MPD_STATE_PAUSE)
		? (int)mpd_status_get_song_id(status)
		: -1;
}

bool
QueuePage::OnSongChange(const struct mpd_status *status)
{
	if (get_current_song_id(status) == current_song_id)
		return false;

	current_song_id = get_current_song_id(status);

	/* center the cursor */
	if (options.auto_center && !lw.HasRangeSelection())
		CenterPlayingItem(status, false);

	return true;
}

#ifndef NCMPC_MINI
static void
add_dir(Completion &completion, const char *dir,
	struct mpdclient &c)
{
	completion.clear();
	gcmp_list_from_path(c, dir, completion, GCMP_TYPE_RFILE);
}

class DatabaseCompletion final : public Completion {
	ScreenManager &screen;
	struct mpdclient &c;
	std::set<std::string> dir_list;

public:
	DatabaseCompletion(ScreenManager &_screen,
			   struct mpdclient &_c) noexcept
		:screen(_screen), c(_c) {}

protected:
	/* virtual methods from class Completion */
	void Pre(std::string_view value) noexcept override;
	void Post(std::string_view value, Range range) noexcept override;
};

void
DatabaseCompletion::Pre(std::string_view line) noexcept
{
	if (empty()) {
		/* create initial list */
		gcmp_list_from_path(c, "", *this, GCMP_TYPE_RFILE);
	} else if (line.ends_with('/')) {
		const auto [it, inserted] = dir_list.emplace(line);
		if (inserted)
			/* add directory content to list */
			add_dir(*this, it->c_str(), c);
	}
}

void
DatabaseCompletion::Post(std::string_view line, Range range) noexcept
{
	if (range.begin() != range.end() &&
	    std::next(range.begin()) != range.end())
		screen_display_completion_list(screen, range);

	if (line.ends_with('/')) {
		/* add directory content to list */
		const auto [it, inserted] = dir_list.emplace(line);
		if (inserted)
			add_dir(*this, it->c_str(), c);
	}
}

#endif

static int
handle_add_to_playlist(ScreenManager &screen, struct mpdclient &c)
{
#ifndef NCMPC_MINI
	/* initialize completion support */
	DatabaseCompletion _completion{screen, c};
	Completion *completion = &_completion;
#else
	Completion *completion = nullptr;
#endif

	/* get path */
	auto path = screen_readln(screen, _("Add"),
				  nullptr,
				  nullptr,
				  completion);

	/* add the path to the playlist */
	if (!path.empty()) {
		mpdclient_cmd_add_path(c, LocaleToUtf8(path.c_str()).c_str());
	}

	return 0;
}

static std::unique_ptr<Page>
screen_queue_init(ScreenManager &_screen, Window window, Size size)
{
	return std::make_unique<QueuePage>(_screen, window, size);
}

inline void
QueuePage::OnHideCursorTimer() noexcept
{
	assert(options.hide_cursor > std::chrono::steady_clock::duration::zero());

	/* hide the cursor when mpd is playing and the user is inactive */

	if (playing) {
		lw.HideCursor();
		Repaint();
	} else
		ScheduleHideCursor();
}

void
QueuePage::OnOpen(struct mpdclient &c) noexcept
{
	playlist = &c.playlist;

	if (options.hide_cursor > std::chrono::steady_clock::duration::zero()) {
		lw.ShowCursor();
		ScheduleHideCursor();
	}

	RestoreSelection();
	OnSongChange(c.status);
}

void
QueuePage::OnClose() noexcept
{
	hide_cursor_timer.Cancel();

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif
}

std::string_view
QueuePage::GetTitle(std::span<char> buffer) const noexcept
{
	if (connection_name.empty())
		return _("Queue");

	return SPrintf(buffer, _("Queue on %s"), connection_name.c_str());
}

void
QueuePage::PaintListItem(const Window window, unsigned i, unsigned y, unsigned width,
			 bool selected) const noexcept
{
	assert(playlist != nullptr);
	assert(i < playlist->size());
	const auto &song = (*playlist)[i];

#ifndef NCMPC_MINI
	if (!song_table_structure.columns.empty()) {
		PaintTableRow(window, width, selected,
			      (int)mpd_song_get_id(&song) == current_song_id,
			      song, table_layout);
		return;
	}
#endif

	class hscroll *row_hscroll = nullptr;
#ifndef NCMPC_MINI
	row_hscroll = selected && options.scroll && lw.GetCursorIndex() == i
		? &hscroll : nullptr;
#endif

	paint_song_row(window, y, width, selected,
		       (int)mpd_song_get_id(&song) == current_song_id,
		       song, row_hscroll, options.list_format.c_str());
}

void
QueuePage::Paint() const noexcept
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif

	lw.Paint(*this);
}

bool
QueuePage::PaintStatusBarOverride(const Window window) const noexcept
{
	if (!lw.HasRangeSelection())
		return false;

	window.MoveCursor({0, 0});
	window.ClearToEol();

	unsigned duration = 0;

	assert(playlist != nullptr);
	for (const unsigned i : lw.GetRange()) {
		assert(i < playlist->size());
		const auto &song = (*playlist)[i];

		duration += mpd_song_get_duration(&song);
	}

	char duration_string[32];
	format_duration_short(duration_string, duration);
	const unsigned duration_width = strlen(duration_string);

	SelectStyle(window, Style::STATUS_TIME);
	window.String({(int)window.GetWidth() - (int)duration_width, 0}, duration_string);

	window.RefreshNoOut();

	return true;
}

void
QueuePage::Update(struct mpdclient &c, unsigned events) noexcept
{
	playing = c.playing;

	if (c.connection_id != last_connection_id) {
		last_connection_id = c.connection_id;
		connection_name = c.GetSettingsName();
	}

	if (events & MPD_IDLE_QUEUE)
		RestoreSelection();
	else
		/* the queue size may have changed, even if we havn't
		   received the QUEUE idle event yet */
		lw.SetLength(playlist->size());

	if (((events & MPD_IDLE_PLAYER) != 0 && OnSongChange(c.status)) ||
	    events & MPD_IDLE_QUEUE)
		/* the queue or the current song has changed, we must
		   paint the new version */
		SetDirty();
}

#ifdef HAVE_GETMOUSE
bool
QueuePage::OnMouse(struct mpdclient &c, Point p, mmask_t bstate)
{
	if (ListPage::OnMouse(c, p, bstate))
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

	if (bstate & BUTTON1_CLICKED) {
		/* play */
		const struct mpd_song *song = GetSelectedSong();
		if (song != nullptr) {
			auto *connection = c.GetConnection();
			if (connection != nullptr &&
			    !mpd_run_play_id(connection,
					     mpd_song_get_id(song)))
				c.HandleError();
		}
	} else if (bstate & (BUTTON3_CLICKED|BUTTON3_DOUBLE_CLICKED)) {
		/* delete */
		if ((bstate & BUTTON3_DOUBLE_CLICKED) ||
		    lw.GetCursorIndex() == old_selected)
			c.RunDelete(lw.GetCursorIndex());

		lw.SetLength(playlist->size());
	}

	SaveSelection();
	SetDirty();

	return true;
}
#endif

bool
QueuePage::OnCommand(struct mpdclient &c, Command cmd)
{
	struct mpd_connection *connection;
	static Command cached_cmd = Command::NONE;

	const Command prev_cmd = cached_cmd;
	cached_cmd = cmd;

	lw.ShowCursor();

	if (options.hide_cursor > std::chrono::steady_clock::duration::zero()) {
		ScheduleHideCursor();
	}

	if (ListPage::OnCommand(c, cmd)) {
		SaveSelection();
		return true;
	}

	switch(cmd) {
	case Command::SCREEN_UPDATE:
		CenterPlayingItem(c.status, prev_cmd == Command::SCREEN_UPDATE);
		SetDirty();
		return false;
	case Command::SELECT_PLAYING:
		if (int pos = c.GetCurrentSongPos(); pos >= 0) {
			lw.SetCursor(pos);
			SaveSelection();
			SetDirty();
			return true;
		} else
			return false;

	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		screen_find(screen, lw, cmd, *this);
		SaveSelection();
		SetDirty();
		return true;
	case Command::LIST_JUMP:
		screen_jump(screen, lw, *this, *this);
		SaveSelection();
		SetDirty();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case Command::SCREEN_SONG:
		if (GetSelectedSong() != nullptr) {
			screen_song_switch(screen, c, *GetSelectedSong());
			return true;
		}

		break;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	case Command::SCREEN_LYRICS:
		if (lw.GetCursorIndex() < playlist->size()) {
			struct mpd_song &selected = (*playlist)[lw.GetCursorIndex()];
			bool follow = false;

			if (&selected == c.GetPlayingSong())
				follow = true;

			screen_lyrics_switch(screen, c, selected, follow);
			return true;
		}

		break;
#endif
	case Command::SCREEN_SWAP:
		if (!playlist->empty())
			screen.Swap(c, &(*playlist)[lw.GetCursorIndex()]);
		else
			screen.Swap(c, nullptr);
		return true;

	default:
		break;
	}

	if (!c.IsConnected())
		return false;

	switch(cmd) {
	case Command::PLAY:
		if (const auto *song = GetSelectedSong()) {
			if (connection = c.GetConnection();
			    connection != nullptr &&
			    !mpd_run_play_id(connection, mpd_song_get_id(song)))
				c.HandleError();

			return true;
		} else
			return false;

	case Command::DELETE: {
		const auto range = lw.GetRange();
		c.RunDeleteRange(range.start_index, range.end_index);

		lw.SetCursor(range.start_index);
		return true;
	}

	case Command::SAVE_PLAYLIST:
		playlist_save(screen, c, nullptr, nullptr);
		return true;

	case Command::ADD:
		handle_add_to_playlist(screen, c);
		return true;

	case Command::SHUFFLE: {
		const auto range = lw.GetRange();
		if (range.end_index <= range.start_index + 1)
			/* No range selection, shuffle all list. */
			break;

		connection = c.GetConnection();
		if (connection == nullptr)
			return true;

		if (mpd_run_shuffle_range(connection, range.start_index,
					  range.end_index))
			screen_status_message(_("Shuffled queue"));
		else
			c.HandleError();
		return true;
	}

	case Command::LIST_MOVE_UP: {
		const auto range = lw.GetRange();
		if (range.start_index == 0 || range.empty())
			return false;

		if (!c.RunMove(range.end_index - 1, range.start_index - 1))
			return true;

		lw.SelectionMovedUp();
		SaveSelection();
		return true;
	}

	case Command::LIST_MOVE_DOWN: {
		const auto range = lw.GetRange();
		if (range.end_index >= playlist->size())
			return false;

		if (!c.RunMove(range.start_index, range.end_index))
			return true;

		lw.SelectionMovedDown();
		SaveSelection();
		return true;
	}

	case Command::LOCATE:
		if (GetSelectedSong() != nullptr) {
			screen_file_goto_song(screen, c, *GetSelectedSong());
			return true;
		}

		break;

	default:
		break;
	}

	return false;
}

const PageMeta screen_queue = {
	"playlist",
	N_("Queue"),
	Command::SCREEN_PLAY,
	screen_queue_init,
};
