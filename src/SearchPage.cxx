/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "SearchPage.hxx"
#include "PageMeta.hxx"
#include "screen_status.hxx"
#include "TextListRenderer.hxx"
#include "i18n.h"
#include "Options.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "screen_utils.hxx"
#include "FileListPage.hxx"
#include "filelist.hxx"

#include <iterator>

#include <string.h>

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
	{ MPD_TAG_COMMENT, "comment", N_("comment") },
	{ MPD_TAG_COUNT, nullptr, nullptr }
};

static int
search_get_tag_id(const char *name)
{
	if (strcasecmp(name, "file") == 0 ||
	    strcasecmp(name, _("file")) == 0)
		return SEARCH_URI;

	if (strcasecmp(name, "modified") == 0)
		return SEARCH_MODIFIED;

	for (unsigned i = 0; search_tag[i].name != nullptr; ++i)
		if (strcasecmp(search_tag[i].name, name) == 0 ||
		    strcasecmp(search_tag[i].localname, name) == 0)
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
	"		name, genre, date composer, performer, comment, file",
	"",
};

static bool advanced_search_mode = false;

class SearchPage final : public FileListPage {
	History search_history;
	std::string pattern;

public:
	SearchPage(ScreenManager &_screen, WINDOW *_w, Size size) noexcept
		:FileListPage(_screen, _w, size,
			      !options.search_format.empty()
			      ? options.search_format.c_str()
			      : options.list_format.c_str()) {
		lw.HideCursor();
		lw.SetLength(std::size(help_text));
	}

private:
	void Clear(bool clear_pattern) noexcept;
	void Reload(struct mpdclient &c);
	void Start(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const noexcept override;
};

/* search info */
class SearchHelpText final : public ListText {
public:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned idx) const noexcept override {
		assert(idx < std::size(help_text));

		if (idx == 0) {
			snprintf(buffer, size,
				 " %s : %s",
				 GetGlobalKeyBindings().GetKeyNames(Command::SCREEN_SEARCH).c_str(),
				 "New search");
			return buffer;
		}

		if (idx == 1) {
			snprintf(buffer, size,
				 " %s : %s [%s]",
				 GetGlobalKeyBindings().GetKeyNames(Command::SEARCH_MODE).c_str(),
				 get_key_description(Command::SEARCH_MODE),
				 gettext(mode[options.search_mode].label));
			return buffer;
		}

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

	SetDirty();
}

static std::unique_ptr<FileList>
search_simple_query(struct mpd_connection *connection, bool exact_match,
		    int table, const char *local_pattern)
{
	const LocaleToUtf8 filter_utf8(local_pattern);

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
search_advanced_query(struct mpd_connection *connection, const char *query)
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
				screen_status_printf(_("Bad search tag %s"),
						     tabv[n]);
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
		screen_status_printf(_("No argument for search tag %s"), tabv[n - 1]);
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
		const LocaleToUtf8 value(matchv[i]);

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
do_search(struct mpdclient *c, const char *query)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return nullptr;

	auto fl = search_advanced_query(connection, query);
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
	filelist = do_search(&c, pattern.c_str());
	if (filelist == nullptr)
		filelist = std::make_unique<FileList>();
	lw.SetLength(filelist->size());

	screen_browser_sync_highlights(*filelist, c.playlist);

	SetDirty();
}

void
SearchPage::Start(struct mpdclient &c)
{
	if (!c.IsConnected())
		return;

	Clear(true);

	pattern = screen_readln(_("Search"),
				nullptr,
				&search_history,
				nullptr);

	if (pattern.empty()) {
		lw.Reset();
		return;
	}

	Reload(c);
}

static std::unique_ptr<Page>
screen_search_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return std::make_unique<SearchPage>(_screen, w, size);
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

const char *
SearchPage::GetTitle(char *str, size_t size) const noexcept
{
	if (advanced_search_mode && !pattern.empty())
		snprintf(str, size, "%s '%s'", _("Search"), pattern.c_str());
	else if (!pattern.empty())
		snprintf(str, size,
			 "%s '%s' [%s]",
			 _("Search"),
			 pattern.c_str(),
			 gettext(mode[options.search_mode].label));
	else
		return _("Search");

	return str;
}

void
SearchPage::Update(struct mpdclient &c, unsigned events) noexcept
{
	if (filelist != nullptr && events & MPD_IDLE_QUEUE) {
		screen_browser_sync_highlights(*filelist, c.playlist);
		SetDirty();
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
		screen_status_printf(_("Search mode: %s"),
				     gettext(mode[options.search_mode].label));

		if (pattern.empty())
			/* show the new mode in the help text */
			SetDirty();
		else if (!advanced_search_mode)
			/* reload only if the new search mode is going
			   to be considered */
			Reload(c);
		return true;

	case Command::SCREEN_UPDATE:
		Reload(c);
		return true;

	case Command::SCREEN_SEARCH:
		Start(c);
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
