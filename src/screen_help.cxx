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

#include "screen_help.hxx"
#include "screen_interface.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"
#include "screen_find.hxx"
#include "paint.hxx"
#include "charset.hxx"
#include "config.h"
#include "i18n.h"

#include <glib.h>

#include <assert.h>

struct help_text_row {
	signed char highlight;
	command_t command;
	const char *text;
};

static const struct help_text_row help_text[] = {
	{ 1, CMD_NONE, N_("Movement") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_LIST_PREVIOUS, nullptr },
	{ 0, CMD_LIST_NEXT, nullptr },
	{ 0, CMD_LIST_TOP, nullptr },
	{ 0, CMD_LIST_MIDDLE, nullptr },
	{ 0, CMD_LIST_BOTTOM, nullptr },
	{ 0, CMD_LIST_PREVIOUS_PAGE, nullptr },
	{ 0, CMD_LIST_NEXT_PAGE, nullptr },
	{ 0, CMD_LIST_FIRST, nullptr },
	{ 0, CMD_LIST_LAST, nullptr },
	{ 0, CMD_LIST_RANGE_SELECT, nullptr },
	{ 0, CMD_LIST_SCROLL_UP_LINE, nullptr},
	{ 0, CMD_LIST_SCROLL_DOWN_LINE, nullptr},
	{ 0, CMD_LIST_SCROLL_UP_HALF, nullptr},
	{ 0, CMD_LIST_SCROLL_DOWN_HALF, nullptr},
	{ 0, CMD_NONE, nullptr },

	{ 0, CMD_SCREEN_PREVIOUS,nullptr },
	{ 0, CMD_SCREEN_NEXT, nullptr },
	{ 0, CMD_SCREEN_SWAP, nullptr },
	{ 0, CMD_SCREEN_HELP, nullptr },
	{ 0, CMD_SCREEN_PLAY, nullptr },
	{ 0, CMD_SCREEN_FILE, nullptr },
#ifdef ENABLE_ARTIST_SCREEN
	{ 0, CMD_SCREEN_ARTIST, nullptr },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ 0, CMD_SCREEN_SEARCH, nullptr },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ 0, CMD_SCREEN_LYRICS, nullptr },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ 0, CMD_SCREEN_OUTPUTS, nullptr },
#endif
#ifdef ENABLE_CHAT_SCREEN
	{ 0, CMD_SCREEN_CHAT, nullptr },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ 0, CMD_SCREEN_KEYDEF, nullptr },
#endif

	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Global") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_STOP, nullptr },
	{ 0, CMD_PAUSE, nullptr },
	{ 0, CMD_CROP, nullptr },
	{ 0, CMD_TRACK_NEXT, nullptr },
	{ 0, CMD_TRACK_PREVIOUS, nullptr },
	{ 0, CMD_SEEK_FORWARD, nullptr },
	{ 0, CMD_SEEK_BACKWARD, nullptr },
	{ 0, CMD_VOLUME_DOWN, nullptr },
	{ 0, CMD_VOLUME_UP, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_REPEAT, nullptr },
	{ 0, CMD_RANDOM, nullptr },
	{ 0, CMD_SINGLE, nullptr },
	{ 0, CMD_CONSUME, nullptr },
	{ 0, CMD_CROSSFADE, nullptr },
	{ 0, CMD_SHUFFLE, nullptr },
	{ 0, CMD_DB_UPDATE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_LIST_FIND, nullptr },
	{ 0, CMD_LIST_RFIND, nullptr },
	{ 0, CMD_LIST_FIND_NEXT, nullptr },
	{ 0, CMD_LIST_RFIND_NEXT, nullptr },
	{ 0, CMD_LIST_JUMP, nullptr },
	{ 0, CMD_TOGGLE_FIND_WRAP, nullptr },
	{ 0, CMD_LOCATE, nullptr },
#ifdef ENABLE_SONG_SCREEN
	{ 0, CMD_SCREEN_SONG, nullptr },
#endif
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_QUIT, nullptr },

	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Queue screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_PLAY, N_("Play") },
	{ 0, CMD_DELETE, nullptr },
	{ 0, CMD_CLEAR, nullptr },
	{ 0, CMD_LIST_MOVE_UP, N_("Move song up") },
	{ 0, CMD_LIST_MOVE_DOWN, N_("Move song down") },
	{ 0, CMD_ADD, nullptr },
	{ 0, CMD_SAVE_PLAYLIST, nullptr },
	{ 0, CMD_SCREEN_UPDATE, N_("Center") },
	{ 0, CMD_SELECT_PLAYING, nullptr },
	{ 0, CMD_TOGGLE_AUTOCENTER, nullptr },

	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Browse screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_PLAY, N_("Enter directory/Select and play song") },
	{ 0, CMD_SELECT, nullptr },
	{ 0, CMD_ADD, N_("Append song to queue") },
	{ 0, CMD_SAVE_PLAYLIST, nullptr },
	{ 0, CMD_DELETE, N_("Delete playlist") },
	{ 0, CMD_GO_PARENT_DIRECTORY, nullptr },
	{ 0, CMD_GO_ROOT_DIRECTORY, nullptr },
	{ 0, CMD_SCREEN_UPDATE, nullptr },

#ifdef ENABLE_SEARCH_SCREEN
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Search screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_SCREEN_SEARCH, N_("Search") },
	{ 0, CMD_PLAY, N_("Select and play") },
	{ 0, CMD_SELECT, nullptr },
	{ 0, CMD_ADD, N_("Append song to queue") },
	{ 0, CMD_SELECT_ALL,	 nullptr },
	{ 0, CMD_SEARCH_MODE, nullptr },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Lyrics screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_SCREEN_LYRICS, N_("View Lyrics") },
	{ 0, CMD_SELECT, N_("(Re)load lyrics") },
	/* to translators: this hotkey aborts the retrieval of lyrics
	   from the server */
	{ 0, CMD_INTERRUPT, N_("Interrupt retrieval") },
	{ 0, CMD_LYRICS_UPDATE, N_("Download lyrics for currently playing song") },
	{ 0, CMD_EDIT, N_("Add or edit lyrics") },
	{ 0, CMD_SAVE_PLAYLIST, N_("Save lyrics") },
	{ 0, CMD_DELETE, N_("Delete saved lyrics") },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Outputs screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_PLAY, N_("Enable/disable output") },
#endif
#ifdef ENABLE_CHAT_SCREEN
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Chat screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_PLAY, N_("Write a message") },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ 0, CMD_NONE, nullptr },
	{ 0, CMD_NONE, nullptr },
	{ 1, CMD_NONE, N_("Keydef screen") },
	{ 2, CMD_NONE, nullptr },
	{ 0, CMD_PLAY, N_("Edit keydefs for selected command") },
	{ 0, CMD_DELETE, N_("Remove selected keydef") },
	{ 0, CMD_ADD, N_("Add a keydef") },
	{ 0, CMD_GO_PARENT_DIRECTORY, N_("Go up a level") },
	{ 0, CMD_SAVE_PLAYLIST, N_("Apply and save changes") },
#endif
};

class HelpPage final : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;

public:
	HelpPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size), screen(_screen) {
		lw.hide_cursor = true;
		lw.SetLength(G_N_ELEMENTS(help_text));
	}

public:
	/* virtual methods from class ListRenderer */
	void PaintListItem(WINDOW *w, unsigned i,
			   unsigned y, unsigned width,
			   bool selected) const override;

	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const override;

	/* virtual methods from class Page */
	void Paint() const override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;

	const char *GetTitle(char *, size_t) const override {
		return _("Help");
	}
};

const char *
HelpPage::GetListItemText(char *, size_t, unsigned i) const
{
	const struct help_text_row *row = &help_text[i];

	assert(i < G_N_ELEMENTS(help_text));

	if (row->text != nullptr)
		return _(row->text);

	if (row->command != CMD_NONE)
		return get_key_description(row->command);

	return "";
}

static Page *
help_init(ScreenManager &screen, WINDOW *w, Size size)
{
	return new HelpPage(screen, w, size);
}

void
HelpPage::PaintListItem(WINDOW *w, unsigned i,
			unsigned y, unsigned width,
			gcc_unused bool selected) const
{
	const struct help_text_row *row = &help_text[i];

	assert(i < G_N_ELEMENTS(help_text));

	row_color(w, row->highlight ? COLOR_LIST_BOLD : COLOR_LIST, false);

	wclrtoeol(w);

	if (row->command == CMD_NONE) {
		if (row->text != nullptr)
			mvwaddstr(w, y, 6, _(row->text));
		else if (row->highlight == 2)
			mvwhline(w, y, 3, '-', width - 6);
	} else {
		const char *key = get_key_names(row->command, true);

		if (utf8_width(key) < 20)
			wmove(w, y, 20 - utf8_width(key));
		waddstr(w, key);
		mvwaddch(w, y, 21, ':');
		mvwaddstr(w, y, 23,
			  row->text != nullptr
			  ? _(row->text)
			  : get_key_description(row->command));
	}
}

void
HelpPage::Paint() const
{
	lw.Paint(*this);
}

bool
HelpPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	lw.SetCursor(lw.start);
	if (screen_find(screen, &lw, cmd, *this)) {
		/* center the row */
		lw.Center(lw.selected);
		SetDirty();
		return true;
	}

	return false;
}

const struct screen_functions screen_help = {
	"help",
	help_init,
};
