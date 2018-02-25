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

#include "screen_queue.hxx"
#include "screen_interface.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "screen_file.hxx"
#include "screen_status.hxx"
#include "screen_find.hxx"
#include "save_playlist.hxx"
#include "config.h"
#include "i18n.h"
#include "charset.hxx"
#include "options.hxx"
#include "mpdclient.hxx"
#include "strfsong.hxx"
#include "wreadln.hxx"
#include "Completion.hxx"
#include "song_paint.hxx"
#include "screen.hxx"
#include "screen_utils.hxx"
#include "screen_song.hxx"
#include "screen_lyrics.hxx"
#include "db_completion.hxx"
#include "Compiler.h"

#ifndef NCMPC_MINI
#include "hscroll.hxx"
#endif

#include <mpd/client.h>

#include <set>
#include <string>

#include <ctype.h>
#include <string.h>
#include <glib.h>

#define MAX_SONG_LENGTH 512

class QueuePage final : public ListPage, ListRenderer {
	ScreenManager &screen;

#ifndef NCMPC_MINI
	mutable class hscroll hscroll;
#endif

	MpdQueue *playlist = nullptr;
	int current_song_id = -1;
	int selected_song_id = -1;
	guint timer_hide_cursor_id = 0;

	unsigned last_connection_id = 0;
	char *connection_name = nullptr;

	bool playing = false;

public:
	QueuePage(ScreenManager &_screen, WINDOW *w,
		  Size size)
		:ListPage(w, size),
		 screen(_screen)
#ifndef NCMPC_MINI
		, hscroll(w, options.scroll_sep)
#endif
	{
	}

	~QueuePage() override {
		g_free(connection_name);
	}

private:
	const struct mpd_song *GetSelectedSong() const;
	void SaveSelection();
	void RestoreSelection();

	void Repaint() const {
		Paint();
		wrefresh(lw.w);
	}

	void CenterPlayingItem(const struct mpd_status *status,
			       bool center_cursor);

	bool OnSongChange(const struct mpd_status *status);

	static gboolean OnHideCursorTimer(gpointer data);

	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i,
			   unsigned y, unsigned width,
			   bool selected) const override;

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void OnClose() override;
	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;

#ifdef HAVE_GETMOUSE
	bool OnMouse(struct mpdclient &c, Point p, mmask_t bstate) override;
#endif

	const char *GetTitle(char *s, size_t size) const override;
};

const struct mpd_song *
QueuePage::GetSelectedSong() const
{
	return !lw.range_selection &&
		lw.selected < playlist->size()
		? &(*playlist)[lw.selected]
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

	int pos = playlist->FindId(selected_song_id);
	if (pos >= 0)
		lw.SetCursor(pos);

	SaveSelection();
}

static const char *
screen_queue_lw_callback(unsigned idx, void *data)
{
	auto &playlist = *(MpdQueue *)data;
	static char songname[MAX_SONG_LENGTH];

	assert(idx < playlist.size());

	const auto &song = playlist[idx];
	strfsong(songname, MAX_SONG_LENGTH, options.list_format, &song);

	return songname;
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

gcc_pure
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
	if (options.auto_center && !lw.range_selection)
		CenterPlayingItem(status, false);

	return true;
}

#ifndef NCMPC_MINI
static void
add_dir(Completion &completion, const char *dir,
	struct mpdclient *c)
{
	completion.clear();
	gcmp_list_from_path(c, dir, completion, GCMP_TYPE_RFILE);
}

class DatabaseCompletion final : public Completion {
	struct mpdclient &c;
	std::set<std::string> dir_list;

public:
	explicit DatabaseCompletion(struct mpdclient &_c)
		:c(_c) {}

protected:
	/* virtual methods from class Completion */
	void Pre(const char *value) override;
	void Post(const char *value, Range range) override;
};

void
DatabaseCompletion::Pre(const char *line)
{
	if (empty()) {
		/* create initial list */
		gcmp_list_from_path(&c, "", *this, GCMP_TYPE_RFILE);
	} else if (line && line[0] && line[strlen(line) - 1] == '/') {
		auto i = dir_list.emplace(line);
		if (i.second)
			/* add directory content to list */
			add_dir(*this, line, &c);
	}
}

void
DatabaseCompletion::Post(const char *line, Range range)
{
	if (range.begin() != range.end() &&
	    std::next(range.begin()) != range.end())
		screen_display_completion_list(range);

	if (line && line[0] && line[strlen(line) - 1] == '/') {
		/* add directory content to list */
		auto i = dir_list.emplace(line);
		if (i.second)
			add_dir(*this, line, &c);
	}
}

#endif

static int
handle_add_to_playlist(struct mpdclient *c)
{
#ifndef NCMPC_MINI
	/* initialize completion support */
	DatabaseCompletion _completion(*c);
	Completion *completion = &_completion;
#else
	Completion *completion = nullptr;
#endif

	/* get path */
	auto path = screen_readln(_("Add"),
				  nullptr,
				  nullptr,
				  completion);

	/* add the path to the playlist */
	if (!path.empty()) {
		mpdclient_cmd_add_path(c, LocaleToUtf8(path.c_str()).c_str());
	}

	return 0;
}

static Page *
screen_queue_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return new QueuePage(_screen, w, size);
}

gboolean
QueuePage::OnHideCursorTimer(gpointer data)
{
	auto &q = *(QueuePage *)data;

	assert(options.hide_cursor > 0);
	assert(q.timer_hide_cursor_id != 0);

	q.timer_hide_cursor_id = 0;

	/* hide the cursor when mpd is playing and the user is inactive */

	if (q.playing) {
		q.lw.hide_cursor = true;
		q.Repaint();
	} else
		q.timer_hide_cursor_id = g_timeout_add_seconds(options.hide_cursor,
							       OnHideCursorTimer, &q);

	return false;
}

void
QueuePage::OnOpen(struct mpdclient &c)
{
	playlist = &c.playlist;

	assert(timer_hide_cursor_id == 0);
	if (options.hide_cursor > 0) {
		lw.hide_cursor = false;
		timer_hide_cursor_id = g_timeout_add_seconds(options.hide_cursor,
							     OnHideCursorTimer, this);
	}

	RestoreSelection();
	OnSongChange(c.status);
}

void
QueuePage::OnClose()
{
	if (timer_hide_cursor_id != 0) {
		g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = 0;
	}

#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif
}

const char *
QueuePage::GetTitle(char *str, size_t size) const
{
       if (connection_name == nullptr)
	       return _("Queue");

       g_snprintf(str, size, _("Queue on %s"), connection_name);
       return str;
}

void
QueuePage::PaintListItem(WINDOW *w, unsigned i, unsigned y, unsigned width,
			 bool selected) const
{
	assert(playlist != nullptr);
	assert(i < playlist->size());
	const auto &song = (*playlist)[i];

	class hscroll *row_hscroll = nullptr;
#ifndef NCMPC_MINI
	row_hscroll = selected && options.scroll && lw.selected == i
		? &hscroll : nullptr;
#endif

	paint_song_row(w, y, width, selected,
		       (int)mpd_song_get_id(&song) == current_song_id,
		       &song, row_hscroll, options.list_format);
}

void
QueuePage::Paint() const
{
#ifndef NCMPC_MINI
	if (options.scroll)
		hscroll.Clear();
#endif

	lw.Paint(*this);
}

void
QueuePage::Update(struct mpdclient &c, unsigned events)
{
	playing = c.playing;

	if (c.connection_id != last_connection_id) {
		last_connection_id = c.connection_id;
		g_free(connection_name);
		connection_name = mpdclient_settings_name(&c);
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
		screen.OnCommand(c, CMD_STOP);
		return true;
	}

	const unsigned old_selected = lw.selected;
	lw.SetCursor(lw.start + p.y);

	if (bstate & BUTTON1_CLICKED) {
		/* play */
		const struct mpd_song *song = GetSelectedSong();
		if (song != nullptr) {
			struct mpd_connection *connection =
				mpdclient_get_connection(&c);

			if (connection != nullptr &&
			    !mpd_run_play_id(connection,
					     mpd_song_get_id(song)))
				mpdclient_handle_error(&c);
		}
	} else if (bstate & BUTTON3_CLICKED) {
		/* delete */
		if (lw.selected == old_selected)
			mpdclient_cmd_delete(&c, lw.selected);

		lw.SetLength(playlist->size());
	}

	SaveSelection();
	SetDirty();

	return true;
}
#endif

bool
QueuePage::OnCommand(struct mpdclient &c, command_t cmd)
{
	struct mpd_connection *connection;
	static command_t cached_cmd = CMD_NONE;

	const command_t prev_cmd = cached_cmd;
	cached_cmd = cmd;

	lw.hide_cursor = false;

	if (options.hide_cursor > 0) {
		if (timer_hide_cursor_id != 0)
			g_source_remove(timer_hide_cursor_id);
		timer_hide_cursor_id = g_timeout_add_seconds(options.hide_cursor,
							     OnHideCursorTimer, this);
	}

	if (ListPage::OnCommand(c, cmd)) {
		SaveSelection();
		return true;
	}

	switch(cmd) {
	case CMD_SCREEN_UPDATE:
		CenterPlayingItem(c.status, prev_cmd == CMD_SCREEN_UPDATE);
		SetDirty();
		return false;
	case CMD_SELECT_PLAYING:
		lw.SetCursor(c.playlist.Find(*c.song));
		SaveSelection();
		SetDirty();
		return true;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(screen, &lw, cmd,
			    screen_queue_lw_callback, &c.playlist);
		SaveSelection();
		SetDirty();
		return true;
	case CMD_LIST_JUMP:
		screen_jump(screen, &lw, screen_queue_lw_callback, &c.playlist,
			    this);
		SaveSelection();
		SetDirty();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		if (GetSelectedSong() != nullptr) {
			screen_song_switch(screen, c, *GetSelectedSong());
			return true;
		}

		break;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		if (lw.selected < c.playlist.size()) {
			struct mpd_song &selected = c.playlist[lw.selected];
			bool follow = false;

			if (c.song &&
			    !strcmp(mpd_song_get_uri(&selected),
				    mpd_song_get_uri(c.song)))
				follow = true;

			screen_lyrics_switch(screen, c, selected, follow);
			return true;
		}

		break;
#endif
	case CMD_SCREEN_SWAP:
		if (!c.playlist.empty())
			screen.Swap(c, &c.playlist[lw.selected]);
		else
			screen.Swap(c, nullptr);
		return true;

	default:
		break;
	}

	if (!c.IsConnected())
		return false;

	switch(cmd) {
		const struct mpd_song *song;
		ListWindowRange range;

	case CMD_PLAY:
		song = GetSelectedSong();
		if (song == nullptr)
			return false;

		connection = mpdclient_get_connection(&c);
		if (connection != nullptr &&
		    !mpd_run_play_id(connection, mpd_song_get_id(song)))
			mpdclient_handle_error(&c);

		return true;

	case CMD_DELETE:
		range = lw.GetRange();
		mpdclient_cmd_delete_range(&c, range.start_index,
					   range.end_index);

		lw.SetCursor(range.start_index);
		return true;

	case CMD_SAVE_PLAYLIST:
		playlist_save(&c, nullptr, nullptr);
		return true;

	case CMD_ADD:
		handle_add_to_playlist(&c);
		return true;

	case CMD_SHUFFLE:
		range = lw.GetRange();
		if (range.end_index <= range.start_index + 1)
			/* No range selection, shuffle all list. */
			break;

		connection = mpdclient_get_connection(&c);
		if (connection == nullptr)
			return true;

		if (mpd_run_shuffle_range(connection, range.start_index,
					  range.end_index))
			screen_status_message(_("Shuffled queue"));
		else
			mpdclient_handle_error(&c);
		return true;

	case CMD_LIST_MOVE_UP:
		range = lw.GetRange();
		if (range.start_index == 0 || range.empty())
			return false;

		if (!mpdclient_cmd_move(&c, range.end_index - 1,
					range.start_index - 1))
			return true;

		lw.selected--;
		lw.range_base--;

		if (lw.range_selection)
			lw.ScrollTo(lw.range_base);
		lw.ScrollTo(lw.selected);

		SaveSelection();
		return true;

	case CMD_LIST_MOVE_DOWN:
		range = lw.GetRange();
		if (range.end_index >= c.playlist.size())
			return false;

		if (!mpdclient_cmd_move(&c, range.start_index,
					range.end_index))
			return true;

		lw.selected++;
		lw.range_base++;

		if (lw.range_selection)
			lw.ScrollTo(lw.range_base);
		lw.ScrollTo(lw.selected);

		SaveSelection();
		return true;

	case CMD_LOCATE:
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

const struct screen_functions screen_queue = {
	"playlist",
	screen_queue_init,
};
