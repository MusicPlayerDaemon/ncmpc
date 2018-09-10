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

#include "HelpPage.hxx"
#include "PageMeta.hxx"
#include "ListPage.hxx"
#include "ListRenderer.hxx"
#include "ListText.hxx"
#include "screen_find.hxx"
#include "paint.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "config.h"
#include "i18n.h"
#include "util/Macros.hxx"
#include "util/StringUTF8.hxx"

#include <assert.h>

struct help_text_row {
	signed char highlight;
	Command command;
	const char *text;
};

static const struct help_text_row help_text[] = {
	{ 1, Command::NONE, N_("Movement") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::LIST_PREVIOUS, nullptr },
	{ 0, Command::LIST_NEXT, nullptr },
	{ 0, Command::LIST_TOP, nullptr },
	{ 0, Command::LIST_MIDDLE, nullptr },
	{ 0, Command::LIST_BOTTOM, nullptr },
	{ 0, Command::LIST_PREVIOUS_PAGE, nullptr },
	{ 0, Command::LIST_NEXT_PAGE, nullptr },
	{ 0, Command::LIST_FIRST, nullptr },
	{ 0, Command::LIST_LAST, nullptr },
	{ 0, Command::LIST_RANGE_SELECT, nullptr },
	{ 0, Command::LIST_SCROLL_UP_LINE, nullptr},
	{ 0, Command::LIST_SCROLL_DOWN_LINE, nullptr},
	{ 0, Command::LIST_SCROLL_UP_HALF, nullptr},
	{ 0, Command::LIST_SCROLL_DOWN_HALF, nullptr},
	{ 0, Command::NONE, nullptr },

	{ 0, Command::SCREEN_PREVIOUS,nullptr },
	{ 0, Command::SCREEN_NEXT, nullptr },
	{ 0, Command::SCREEN_SWAP, nullptr },
	{ 0, Command::SCREEN_HELP, nullptr },
	{ 0, Command::SCREEN_PLAY, nullptr },
	{ 0, Command::SCREEN_FILE, nullptr },
#ifdef ENABLE_ARTIST_SCREEN
	{ 0, Command::SCREEN_ARTIST, nullptr },
#endif
#ifdef ENABLE_SEARCH_SCREEN
	{ 0, Command::SCREEN_SEARCH, nullptr },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ 0, Command::SCREEN_LYRICS, nullptr },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ 0, Command::SCREEN_OUTPUTS, nullptr },
#endif
#ifdef ENABLE_CHAT_SCREEN
	{ 0, Command::SCREEN_CHAT, nullptr },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ 0, Command::SCREEN_KEYDEF, nullptr },
#endif

	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Global") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::STOP, nullptr },
	{ 0, Command::PAUSE, nullptr },
	{ 0, Command::CROP, nullptr },
	{ 0, Command::TRACK_NEXT, nullptr },
	{ 0, Command::TRACK_PREVIOUS, nullptr },
	{ 0, Command::SEEK_FORWARD, nullptr },
	{ 0, Command::SEEK_BACKWARD, nullptr },
	{ 0, Command::VOLUME_DOWN, nullptr },
	{ 0, Command::VOLUME_UP, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 0, Command::REPEAT, nullptr },
	{ 0, Command::RANDOM, nullptr },
	{ 0, Command::SINGLE, nullptr },
	{ 0, Command::CONSUME, nullptr },
	{ 0, Command::CROSSFADE, nullptr },
	{ 0, Command::SHUFFLE, nullptr },
	{ 0, Command::DB_UPDATE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 0, Command::LIST_FIND, nullptr },
	{ 0, Command::LIST_RFIND, nullptr },
	{ 0, Command::LIST_FIND_NEXT, nullptr },
	{ 0, Command::LIST_RFIND_NEXT, nullptr },
	{ 0, Command::LIST_JUMP, nullptr },
	{ 0, Command::TOGGLE_FIND_WRAP, nullptr },
	{ 0, Command::LOCATE, nullptr },
#ifdef ENABLE_SONG_SCREEN
	{ 0, Command::SCREEN_SONG, nullptr },
#endif
	{ 0, Command::NONE, nullptr },
	{ 0, Command::QUIT, nullptr },

	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Queue screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::PLAY, N_("Play") },
	{ 0, Command::DELETE, nullptr },
	{ 0, Command::CLEAR, nullptr },
	{ 0, Command::LIST_MOVE_UP, N_("Move song up") },
	{ 0, Command::LIST_MOVE_DOWN, N_("Move song down") },
	{ 0, Command::ADD, nullptr },
	{ 0, Command::SAVE_PLAYLIST, nullptr },
	{ 0, Command::SCREEN_UPDATE, N_("Center") },
	{ 0, Command::SELECT_PLAYING, nullptr },
	{ 0, Command::TOGGLE_AUTOCENTER, nullptr },

	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Browse screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::PLAY, N_("Enter directory/Select and play song") },
	{ 0, Command::SELECT, nullptr },
	{ 0, Command::ADD, N_("Append song to queue") },
	{ 0, Command::SAVE_PLAYLIST, nullptr },
	{ 0, Command::DELETE, N_("Delete playlist") },
	{ 0, Command::GO_PARENT_DIRECTORY, nullptr },
	{ 0, Command::GO_ROOT_DIRECTORY, nullptr },
	{ 0, Command::SCREEN_UPDATE, nullptr },

#ifdef ENABLE_SEARCH_SCREEN
	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Search screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::SCREEN_SEARCH, N_("Search") },
	{ 0, Command::PLAY, N_("Select and play") },
	{ 0, Command::SELECT, nullptr },
	{ 0, Command::ADD, N_("Append song to queue") },
	{ 0, Command::SELECT_ALL,	 nullptr },
	{ 0, Command::SEARCH_MODE, nullptr },
#endif
#ifdef ENABLE_LYRICS_SCREEN
	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Lyrics screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::SCREEN_LYRICS, N_("View Lyrics") },
	{ 0, Command::SELECT, N_("(Re)load lyrics") },
	/* to translators: this hotkey aborts the retrieval of lyrics
	   from the server */
	{ 0, Command::INTERRUPT, N_("Interrupt retrieval") },
	{ 0, Command::LYRICS_UPDATE, N_("Download lyrics for currently playing song") },
	{ 0, Command::EDIT, N_("Add or edit lyrics") },
	{ 0, Command::SAVE_PLAYLIST, N_("Save lyrics") },
	{ 0, Command::DELETE, N_("Delete saved lyrics") },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Outputs screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::PLAY, N_("Enable/disable output") },
#endif
#ifdef ENABLE_CHAT_SCREEN
	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Chat screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::PLAY, N_("Write a message") },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	{ 0, Command::NONE, nullptr },
	{ 0, Command::NONE, nullptr },
	{ 1, Command::NONE, N_("Keydef screen") },
	{ 2, Command::NONE, nullptr },
	{ 0, Command::PLAY, N_("Edit keydefs for selected command") },
	{ 0, Command::DELETE, N_("Remove selected keydef") },
	{ 0, Command::ADD, N_("Add a keydef") },
	{ 0, Command::GO_PARENT_DIRECTORY, N_("Go up a level") },
	{ 0, Command::SAVE_PLAYLIST, N_("Apply and save changes") },
#endif
};

class HelpPage final : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;

public:
	HelpPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size), screen(_screen) {
		lw.hide_cursor = true;
		lw.SetLength(ARRAY_SIZE(help_text));
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
	bool OnCommand(struct mpdclient &c, Command cmd) override;

	const char *GetTitle(char *, size_t) const override {
		return _("Help");
	}
};

const char *
HelpPage::GetListItemText(char *, size_t, unsigned i) const
{
	const struct help_text_row *row = &help_text[i];

	assert(i < ARRAY_SIZE(help_text));

	if (row->text != nullptr)
		return _(row->text);

	if (row->command != Command::NONE)
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

	assert(i < ARRAY_SIZE(help_text));

	row_color(w, row->highlight ? COLOR_LIST_BOLD : COLOR_LIST, false);

	wclrtoeol(w);

	if (row->command == Command::NONE) {
		if (row->text != nullptr)
			mvwaddstr(w, y, 6, _(row->text));
		else if (row->highlight == 2)
			mvwhline(w, y, 3, '-', width - 6);
	} else {
		const char *key =
			GetGlobalKeyBindings().GetKeyNames(row->command, true);

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
HelpPage::OnCommand(struct mpdclient &c, Command cmd)
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

const PageMeta screen_help = {
	"help",
	help_init,
};
