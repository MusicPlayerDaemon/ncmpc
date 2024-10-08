// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "SearchPage.hxx"
#include "PageMeta.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "Options.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "charset.hxx"
#include "FileListPage.hxx"
#include "filelist.hxx"
#include "dialogs/TextInputDialog.hxx"
#include "ui/TextListRenderer.hxx"
#include "lib/fmt/ToSpan.hxx"
#include "client/mpdclient.hxx"
#include "util/StringAPI.hxx"

#include <iterator>

#include <string.h>

using std::string_view_literals::operator""sv;

enum {
	SEARCH_URI = MPD_TAG_COUNT + 100,
	SEARCH_MODIFIED,
	SEARCH_ARTIST_TITLE,
};

static constexpr struct {
	enum mpd_tag_type tag_type;
	const char *name;
	const char *localname;
} search_tag[MPD_TAG_COUNT] = {
	{ MPD_TAG_ARTIST, "artist", N_("artist") },
	{ MPD_TAG_ALBUM, "album", N_("album") },
	{ MPD_TAG_TITLE, "title", N_("title") },
	{ MPD_TAG_TRACK, "track", N_("track") },
	{ MPD_TAG_NAME, "name", N_("name") },
	{ MPD_TAG_GENRE, "genre", N_("genre") },
	{ MPD_TAG_DATE, "date", N_("date") },
	{ MPD_TAG_COMPOSER, "composer", N_("composer") },
	{ MPD_TAG_PERFORMER, "performer", N_("performer") },
	{ MPD_TAG_LABEL, "label", N_("label") },
	{ MPD_TAG_COMMENT, "comment", N_("comment") },
	{ MPD_TAG_COUNT, nullptr, nullptr }
};

static int
search_get_tag_id(const char *name)
{
	if (StringIsEqualIgnoreCase(name, "file") ||
	    StringIsEqualIgnoreCase(name, _("file")))
		return SEARCH_URI;

	if (StringIsEqualIgnoreCase(name, "modified"))
		return SEARCH_MODIFIED;

	for (unsigned i = 0; search_tag[i].name != nullptr; ++i)
		if (StringIsEqualIgnoreCase(search_tag[i].name, name) ||
		    StringIsEqualIgnoreCase(search_tag[i].localname, name))
			return search_tag[i].tag_type;

	return -1;
}

struct SearchMode {
	int table;
	const char *label;
};

static constexpr SearchMode mode[] = {
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ SEARCH_URI, N_("Filename") },
	{ SEARCH_ARTIST_TITLE, N_("Artist + Title") },
	{ MPD_TAG_COUNT, nullptr }
};

static const char *const help_text[] = {
	"",
	"",
	"",
	"Quick     -  Enter a string and ncmpc will search according",
	"             to the current search mode (displayed above).",
	"",
	"Advanced  -  <tag>:<search term> [<tag>:<search term>...]",
	"		Example: artist:radiohead album:pablo honey",
	"		Example: modified:14d (units: s, M, h, d, m, y)",
	"",
	"		Available tags: artist, album, title, track,",
	"		name, genre, date composer, performer, label,",
	"		comment, file",
	"",
};

static bool advanced_search_mode = false;

class SearchPage final : public FileListPage {
	History search_history;
	std::string pattern;

public:
	SearchPage(ScreenManager &_screen, const Window _window) noexcept
		:FileListPage(_screen, _screen, _window,
			      !options.search_format.empty()
			      ? options.search_format.c_str()
			      : options.list_format.c_str()) {
		lw.HideCursor();
		lw.SetLength(std::size(help_text));
	}

private:
	void Clear(bool clear_pattern) noexcept;
	void Reload(struct mpdclient &c);

	[[nodiscard]]
	Co::InvokeTask Start(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;
};

/* search info */
class SearchHelpText final : public ListText {
public:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned idx) const noexcept override {
		assert(idx < std::size(help_text));

		if (idx == 0)
			return FmtTruncate(buffer, " {} : {}"sv,
					   GetGlobalKeyBindings().GetKeyNames(Command::SCREEN_SEARCH),
					   "New search"sv);

		if (idx == 1)
			return FmtTruncate(buffer, " {} : {} [{}]"sv,
					   GetGlobalKeyBindings().GetKeyNames(Command::SEARCH_MODE),
					   get_key_description(Command::SEARCH_MODE),
					   my_gettext(mode[options.search_mode].label));

		return help_text[idx];
	}
};

void
SearchPage::Clear(bool clear_pattern) noexcept
{
	if (filelist) {
		filelist = std::make_unique<FileList>();
		lw.SetLength(0);
	}
	if (clear_pattern)
		pattern.clear();

	SchedulePaint();
}

static std::unique_ptr<FileList>
search_simple_query(struct mpd_connection *connection, bool exact_match,
		    int table, const char *local_pattern)
{
	const LocaleToUtf8Z filter_utf8(local_pattern);

	if (table == SEARCH_ARTIST_TITLE) {
		mpd_command_list_begin(connection, false);

		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST,
					      filter_utf8.c_str());
		mpd_search_commit(connection);

		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_TITLE,
					      filter_utf8.c_str());
		mpd_search_commit(connection);

		mpd_command_list_end(connection);

		auto list = filelist_new_recv(connection);
		list->RemoveDuplicateSongs();
		return list;
	} else if (table == SEARCH_URI) {
		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_uri_constraint(connection, MPD_OPERATOR_DEFAULT,
					      filter_utf8.c_str());
		mpd_search_commit(connection);

		return filelist_new_recv(connection);
	} else {
		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      (enum mpd_tag_type)table,
					      filter_utf8.c_str());
		mpd_search_commit(connection);

		return filelist_new_recv(connection);
	}
}

/**
 * Throws on error.
 */
static time_t
ParseModifiedSince(const char *s)
{
	char *endptr;
	time_t value = strtoul(s, &endptr, 10);
	if (endptr == s)
		throw _("Invalid number");

	constexpr time_t MINUTE = 60;
	constexpr time_t HOUR = 60 * MINUTE;
	constexpr time_t DAY = 24 * HOUR;
	constexpr time_t MONTH = 30 * DAY; // TODO: inaccurate
	constexpr time_t YEAR = 365 * DAY; // TODO: inaccurate

	s = endptr;
	switch (*s) {
	case 's':
		++s;
		break;

	case 'M':
		++s;
		value *= MINUTE;
		break;

	case 'h':
		++s;
		value *= HOUR;
		break;

	case 'd':
		++s;
		value *= DAY;
		break;

	case 'm':
		++s;
		value *= MONTH;
		break;

	case 'y':
	case 'Y':
		++s;
		value *= YEAR;
		break;

	default:
		throw _("Unrecognized suffix");
	}

	if (*s != '\0')
		throw _("Unrecognized suffix");

	return time(nullptr) - value;
}

/*-----------------------------------------------------------------------
 * NOTE: This code exists to test a new search ui,
 *       Its ugly and MUST be redesigned before the next release!
 *-----------------------------------------------------------------------
 */
static std::unique_ptr<FileList>
search_advanced_query(Interface &interface,
		      struct mpd_connection *connection, const char *query)
try {
	advanced_search_mode = false;
	if (strchr(query, ':') == nullptr)
		return nullptr;

	std::string str(query);

	static constexpr size_t N = 10;

	char *tabv[N];
	char *matchv[N];
	int table[N];

	/*
	 * Replace every : with a '\0' and every space character
	 * before it unless spi = -1, link the resulting strings
	 * to their proper vector.
	 */
	int spi = -1;
	size_t n = 0;
	for (size_t i = 0; str[i] != '\0' && n < N; i++) {
		switch(str[i]) {
		case ' ':
			spi = i;
			continue;
		case ':':
			str[i] = '\0';
			if (spi != -1)
				str[spi] = '\0';

			matchv[n] = &str[i + 1];
			tabv[n] = &str[spi + 1];
			table[n] = search_get_tag_id(tabv[n]);
			if (table[n] < 0) {
				interface.Alert(fmt::format(fmt::runtime(_("Bad search tag {}")),
							    tabv[n]));
				return nullptr;
			}

			++n;
			/* FALLTHROUGH */
		default:
			continue;
		}
	}

	/* Get rid of obvious failure case */
	if (matchv[n - 1][0] == '\0') {
		interface.Alert(fmt::format(fmt::runtime(_("No argument for search tag {}")),
					    tabv[n - 1]));
		return nullptr;
	}

	advanced_search_mode = true;

	/*-----------------------------------------------------------------------
	 * NOTE (again): This code exists to test a new search ui,
	 *               Its ugly and MUST be redesigned before the next release!
	 *             + the code below should live in mpdclient.c
	 *-----------------------------------------------------------------------
	 */
	/** stupid - but this is just a test...... (fulhack)  */
	mpd_search_db_songs(connection, false);

	for (size_t i = 0; i < n; i++) {
		const LocaleToUtf8Z value{matchv[i]};

		if (table[i] == SEARCH_URI)
			mpd_search_add_uri_constraint(connection,
						      MPD_OPERATOR_DEFAULT,
						      value.c_str());
		else if (table[i] == SEARCH_MODIFIED)
			mpd_search_add_modified_since_constraint(connection,
								 MPD_OPERATOR_DEFAULT,
								 ParseModifiedSince(value.c_str()));
		else
			mpd_search_add_tag_constraint(connection,
						      MPD_OPERATOR_DEFAULT,
						      (enum mpd_tag_type)table[i],
						      value.c_str());
	}

	mpd_search_commit(connection);
	auto fl = filelist_new_recv(connection);
	if (!mpd_response_finish(connection))
		fl.reset();

	return fl;
} catch (...) {
	mpd_search_cancel(connection);
	throw;
}

static std::unique_ptr<FileList>
do_search(Interface &interface,
	  struct mpdclient *c, const char *query)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return nullptr;

	auto fl = search_advanced_query(interface, connection, query);
	if (fl != nullptr)
		return fl;

	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		c->HandleError();
		return nullptr;
	}

	fl = search_simple_query(connection, false,
				 mode[options.search_mode].table,
				 query);
	if (fl == nullptr)
		c->HandleError();
	return fl;
}

void
SearchPage::Reload(struct mpdclient &c)
{
	if (pattern.empty())
		return;

	lw.ShowCursor();
	filelist = do_search(GetInterface(), &c, pattern.c_str());
	if (filelist == nullptr)
		filelist = std::make_unique<FileList>();
	lw.SetLength(filelist->size());

	screen_browser_sync_highlights(*filelist, c.playlist);

	SchedulePaint();
}

inline Co::InvokeTask
SearchPage::Start(struct mpdclient &c)
{
	if (!c.IsReady())
		co_return;

	Clear(true);

	pattern = co_await TextInputDialog{
		screen, _("Search"),
		{},
		{ .history = &search_history },
	};

	if (pattern.empty()) {
		lw.Reset();
		co_return;
	}

	Reload(c);
}

static std::unique_ptr<Page>
screen_search_init(ScreenManager &_screen, const Window window)
{
	return std::make_unique<SearchPage>(_screen, window);
}

void
SearchPage::Paint() const noexcept
{
	if (filelist) {
		FileListPage::Paint();
	} else {
		lw.Paint(TextListRenderer(SearchHelpText()));
	}
}

std::string_view
SearchPage::GetTitle(std::span<char> buffer) const noexcept
{
	if (advanced_search_mode && !pattern.empty())
		return FmtTruncate(buffer, "{} '{}'"sv, _("Search"), pattern);
	else if (!pattern.empty())
		return FmtTruncate(buffer, "{} '{}' [{}]",
				   _("Search"),
				   pattern,
				   my_gettext(mode[options.search_mode].label));
	else
		return _("Search");
}

void
SearchPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (filelist != nullptr && events & MPD_IDLE_QUEUE) {
		screen_browser_sync_highlights(*filelist, c.playlist);
		SchedulePaint();
	}
}

bool
SearchPage::OnCommand(struct mpdclient &c, Command cmd)
{
	switch (cmd) {
	case Command::SEARCH_MODE:
		options.search_mode++;
		if (mode[options.search_mode].label == nullptr)
			options.search_mode = 0;
		FmtAlert("{}: {}"sv, _("Search mode"),
			 my_gettext(mode[options.search_mode].label));

		if (pattern.empty())
			/* show the new mode in the help text */
			SchedulePaint();
		else if (!advanced_search_mode)
			/* reload only if the new search mode is going
			   to be considered */
			Reload(c);
		return true;

	case Command::SCREEN_UPDATE:
		Reload(c);
		return true;

	case Command::SCREEN_SEARCH:
		CoStart(Start(c));
		return true;

	case Command::CLEAR:
		Clear(true);
		lw.Reset();
		return true;

	default:
		break;
	}

	if (FileListPage::OnCommand(c, cmd))
		return true;

	return false;
}

const PageMeta screen_search = {
	"search",
	N_("Search"),
	Command::SCREEN_SEARCH,
	screen_search_init,
};
