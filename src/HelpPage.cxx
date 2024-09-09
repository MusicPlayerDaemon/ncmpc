// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "HelpPage.hxx"
#include "PageMeta.hxx"
#include "screen_find.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "config.h"
#include "i18n.h"
#include "screen.hxx"
#include "page/ListPage.hxx"
#include "ui/ListRenderer.hxx"
#include "ui/ListText.hxx"
#include "ui/paint.hxx"
#include "util/LocaleString.hxx"

#include <iterator>

#include <assert.h>

struct HelpRow {
	signed char highlight;
	Command command;
	const char *text;

	constexpr HelpRow(signed char _highlight,
			  Command _command,
			  const char *_text=nullptr) noexcept
		:highlight(_highlight), command(_command), text(_text) {}

	constexpr HelpRow(Command _command,
			  const char *_text=nullptr) noexcept
		:HelpRow(0, _command, _text) {}
};

static constexpr HelpRow HLINE{2, Command::NONE};

static constexpr HelpRow
Heading(const char *text) noexcept
{
	return {1, Command::NONE, text};
}

static constexpr HelpRow help_text[] = {
	Heading(N_("Movement")),
	HLINE,
	Command::LIST_PREVIOUS,
	Command::LIST_NEXT,
	Command::LIST_TOP,
	Command::LIST_MIDDLE,
	Command::LIST_BOTTOM,
	Command::LIST_PREVIOUS_PAGE,
	Command::LIST_NEXT_PAGE,
	Command::LIST_FIRST,
	Command::LIST_LAST,
	Command::LIST_RANGE_SELECT,
	Command::LIST_SCROLL_UP_LINE,
	Command::LIST_SCROLL_DOWN_LINE,
	Command::LIST_SCROLL_UP_HALF,
	Command::LIST_SCROLL_DOWN_HALF,
	Command::NONE,

	Command::SCREEN_PREVIOUS,
	Command::SCREEN_NEXT,
	Command::SCREEN_SWAP,
	Command::SCREEN_HELP,
	Command::SCREEN_PLAY,
	Command::SCREEN_FILE,
#ifdef ENABLE_LIBRARY_PAGE
	Command::LIBRARY_PAGE,
#endif
#ifdef ENABLE_SEARCH_SCREEN
	Command::SCREEN_SEARCH,
#endif
#ifdef ENABLE_LYRICS_SCREEN
	Command::SCREEN_LYRICS,
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	Command::SCREEN_OUTPUTS,
#endif
#ifdef ENABLE_CHAT_SCREEN
	Command::SCREEN_CHAT,
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	Command::SCREEN_KEYDEF,
#endif

	Command::NONE,
	Command::NONE,
	Heading(N_("Global")),
	HLINE,
	Command::STOP,
	Command::PAUSE,
	Command::CROP,
	Command::TRACK_NEXT,
	Command::TRACK_PREVIOUS,
	Command::SEEK_FORWARD,
	Command::SEEK_BACKWARD,
	Command::VOLUME_DOWN,
	Command::VOLUME_UP,
	Command::NONE,
	Command::REPEAT,
	Command::RANDOM,
	Command::SINGLE,
	Command::CONSUME,
	Command::CROSSFADE,
	Command::SHUFFLE,
	Command::DB_UPDATE,
	Command::NONE,
	Command::LIST_FIND,
	Command::LIST_RFIND,
	Command::LIST_FIND_NEXT,
	Command::LIST_RFIND_NEXT,
	Command::LIST_JUMP,
	Command::TOGGLE_FIND_WRAP,
	Command::LOCATE,
#ifdef ENABLE_SONG_SCREEN
	Command::SCREEN_SONG,
#endif
	Command::NONE,
	Command::QUIT,

	Command::NONE,
	Command::NONE,
	Heading(N_("Queue screen")),
	HLINE,
	{ Command::PLAY, N_("Play") },
	Command::DELETE,
	Command::CLEAR,
	Command::LIST_MOVE_UP,
	Command::LIST_MOVE_DOWN,
	Command::ADD,
	Command::SAVE_PLAYLIST,
	{ Command::SCREEN_UPDATE, N_("Center") },
	Command::SELECT_PLAYING,
	Command::TOGGLE_AUTOCENTER,

	Command::NONE,
	Command::NONE,
	Heading(N_("Browse screen")),
	HLINE,
	{ Command::PLAY, N_("Enter directory/Select and play song") },
	Command::SELECT,
	Command::ADD,
#ifdef ENABLE_PLAYLIST_EDITOR
	Command::EDIT,
#endif
	Command::SAVE_PLAYLIST,
	{ Command::DELETE, N_("Delete playlist") },
	Command::GO_PARENT_DIRECTORY,
	Command::GO_ROOT_DIRECTORY,
	Command::SCREEN_UPDATE,

#ifdef ENABLE_SEARCH_SCREEN
	Command::NONE,
	Command::NONE,
	Heading(N_("Search screen")),
	HLINE,
	{ Command::SCREEN_SEARCH, N_("New search") },
	{ Command::PLAY, N_("Select and play") },
	Command::SELECT,
	{ Command::ADD, N_("Append song to queue") },
	Command::SELECT_ALL,
	Command::SEARCH_MODE,
#endif
#ifdef ENABLE_LYRICS_SCREEN
	Command::NONE,
	Command::NONE,
	Heading(N_("Lyrics screen")),
	HLINE,
	{ Command::SCREEN_LYRICS, N_("View Lyrics") },
	{ Command::SELECT, N_("(Re)load lyrics") },
	/* to translators: this hotkey aborts the retrieval of lyrics
	   from the server */
	{ Command::INTERRUPT, N_("Interrupt retrieval") },
	{ Command::LYRICS_UPDATE, N_("Download lyrics for currently playing song") },
	{ Command::EDIT, N_("Add or edit lyrics") },
	{ Command::SAVE_PLAYLIST, N_("Save lyrics") },
	{ Command::DELETE, N_("Delete saved lyrics") },
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
	Command::NONE,
	Command::NONE,
	Heading(N_("Outputs screen")),
	HLINE,
	{ Command::PLAY, N_("Enable/disable output") },
#endif
#ifdef ENABLE_CHAT_SCREEN
	Command::NONE,
	Command::NONE,
	Heading(N_("Chat screen")),
	HLINE,
	{ Command::PLAY, N_("Write a message") },
#endif
#ifdef ENABLE_KEYDEF_SCREEN
	Command::NONE,
	Command::NONE,
	Heading(N_("Keydef screen")),
	HLINE,
	{ Command::PLAY, N_("Edit keydefs for selected command") },
	{ Command::DELETE, N_("Remove selected keydef") },
	{ Command::ADD, N_("Add a keydef") },
	{ Command::GO_PARENT_DIRECTORY, N_("Go up a level") },
	{ Command::SAVE_PLAYLIST, N_("Apply and save changes") },
#endif
};

class HelpPage final : public ListPage, ListRenderer, ListText {
	ScreenManager &screen;

public:
	HelpPage(ScreenManager &_screen, const Window _window, Size size)
		:ListPage(_screen, _window, size), screen(_screen) {
		lw.HideCursor();
		lw.SetLength(std::size(help_text));
	}

public:
	/* virtual methods from class ListRenderer */
	void PaintListItem(const Window window, unsigned i,
			   unsigned y, unsigned width,
			   bool selected) const noexcept override;

	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;

	/* virtual methods from class Page */
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

	std::string_view GetTitle(std::span<char>) const noexcept override {
		return _("Help");
	}
};

std::string_view
HelpPage::GetListItemText(std::span<char>, unsigned i) const noexcept
{
	const auto *row = &help_text[i];

	assert(i < std::size(help_text));

	if (row->text != nullptr)
		return my_gettext(row->text);

	if (row->command != Command::NONE)
		return get_key_description(row->command);

	return {};
}

static std::unique_ptr<Page>
help_init(ScreenManager &screen, const Window window, Size size)
{
	return std::make_unique<HelpPage>(screen, window, size);
}

void
HelpPage::PaintListItem(const Window window, unsigned i,
			unsigned y, unsigned width,
			bool selected) const noexcept
{
	const auto *row = &help_text[i];

	assert(i < std::size(help_text));

	row_color(window, row->highlight ? Style::LIST_BOLD : Style::LIST,
		  selected);

	window.ClearToEol();

	if (row->command == Command::NONE) {
		if (row->text != nullptr)
			window.String({6, (int)y}, my_gettext(row->text));
		else if (row->highlight == 2)
			window.HLine({3, (int)y}, width - 6, ACS_HLINE);
	} else {
		const auto key =
			GetGlobalKeyBindings().GetKeyNames(row->command);

		if (const auto key_width = StringWidthMB(key); key_width < 20)
			window.MoveCursor({20 - (int)key_width, (int)y});
		window.String(key);
		window.Char({21, (int)y}, ':');
		window.String({23, (int)y},
			      row->text != nullptr
			      ? my_gettext(row->text)
			      : get_key_description(row->command));
	}
}

void
HelpPage::Paint() const noexcept
{
	lw.Paint(*this);
}

bool
HelpPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	if (!lw.IsCursorVisible())
		/* start searching at the beginning of the page (not
		   where the invisible cursor just happens to be),
		   unless the cursor is still visible from the last
		   search */
		lw.SetCursorFromOrigin(0);

	if (screen_find(screen, lw, cmd, *this)) {
		SchedulePaint();
		return true;
	}

	return false;
}

const PageMeta screen_help = {
	"help",
	N_("Help"),
	Command::SCREEN_HELP,
	help_init,
};
