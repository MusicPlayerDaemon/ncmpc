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

#include "SearchPage.hxx"
#include "PageMeta.hxx"
#include "screen_status.hxx"
#include "TextListRenderer.hxx"
#include "i18n.h"
#include "options.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "strfsong.hxx"
#include "screen_utils.hxx"
#include "FileListPage.hxx"
#include "filelist.hxx"
#include "util/Macros.hxx"

#include <glib.h>

#include <algorithm>

#include <string.h>

enum {
	SEARCH_URI = MPD_TAG_COUNT + 100,
	SEARCH_ARTIST_TITLE
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

	for (unsigned i = 0; search_tag[i].name != nullptr; ++i)
		if (strcasecmp(search_tag[i].name, name) == 0 ||
		    strcasecmp(search_tag[i].localname, name) == 0)
			return search_tag[i].tag_type;

	return -1;
}

typedef struct {
	enum mpd_tag_type table;
	const char *label;
} search_type_t;

static constexpr search_type_t mode[] = {
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ (enum mpd_tag_type)SEARCH_URI, N_("Filename") },
	{ (enum mpd_tag_type)SEARCH_ARTIST_TITLE, N_("Artist + Title") },
	{ MPD_TAG_COUNT, nullptr }
};

static const char *const help_text[] = {
	"Quick     -  Enter a string and ncmpc will search according",
	"             to the current search mode (displayed above).",
	"",
	"Advanced  -  <tag>:<search term> [<tag>:<search term>...]",
	"		Example: artist:radiohead album:pablo honey",
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
	SearchPage(ScreenManager &_screen, WINDOW *_w, Size size)
		:FileListPage(_screen, _w, size,
			      !options.search_format.empty()
			      ? options.search_format.c_str()
			      : options.list_format.c_str()) {
		lw.SetLength(ARRAY_SIZE(help_text));
		lw.hide_cursor = true;
	}

private:
	void Clear(bool clear_pattern);
	void Reload(struct mpdclient &c);
	void Start(struct mpdclient &c);

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;
	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

/* search info */
class SearchHelpText final : public ListText {
public:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *, size_t, unsigned idx) const override {
		assert(idx < ARRAY_SIZE(help_text));

		return help_text[idx];
	}
};

void
SearchPage::Clear(bool clear_pattern)
{
	if (filelist) {
		delete filelist;
		filelist = new FileList();
		lw.SetLength(0);
	}
	if (clear_pattern)
		pattern.clear();

	SetDirty();
}

static FileList *
search_simple_query(struct mpd_connection *connection, bool exact_match,
		    int table, const char *local_pattern)
{
	FileList *list;
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

		list = filelist_new_recv(connection);
		list->RemoveDuplicateSongs();
	} else if (table == SEARCH_URI) {
		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_uri_constraint(connection, MPD_OPERATOR_DEFAULT,
					      filter_utf8.c_str());
		mpd_search_commit(connection);

		list = filelist_new_recv(connection);
	} else {
		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      (enum mpd_tag_type)table,
					      filter_utf8.c_str());
		mpd_search_commit(connection);

		list = filelist_new_recv(connection);
	}

	return list;
}

/*-----------------------------------------------------------------------
 * NOTE: This code exists to test a new search ui,
 *       Its ugly and MUST be redesigned before the next release!
 *-----------------------------------------------------------------------
 */
static FileList *
search_advanced_query(struct mpd_connection *connection, const char *query)
{
	advanced_search_mode = false;
	if (strchr(query, ':') == nullptr)
		return nullptr;

	char *str = g_strdup(query);

	static constexpr size_t N = 10;

	char *tabv[N];
	char *matchv[N];
	int table[N];
	char *arg[N];

	std::fill_n(tabv, N, nullptr);
	std::fill_n(matchv, N, nullptr);
	std::fill_n(table, N, -1);
	std::fill_n(arg, N, nullptr);

	/*
	 * Replace every : with a '\0' and every space character
	 * before it unless spi = -1, link the resulting strings
	 * to their proper vector.
	 */
	int spi = -1;
	size_t j = 0;
	for (size_t i = 0; str[i] != '\0' && j < N; i++) {
		switch(str[i]) {
		case ' ':
			spi = i;
			continue;
		case ':':
			str[i] = '\0';
			if (spi != -1)
				str[spi] = '\0';

			matchv[j] = str + i + 1;
			tabv[j] = str + spi + 1;
			j++;
			/* FALLTHROUGH */
		default:
			continue;
		}
	}

	/* Get rid of obvious failure case */
	if (matchv[j - 1][0] == '\0') {
		screen_status_printf(_("No argument for search tag %s"), tabv[j - 1]);
		g_free(str);
		return nullptr;
	}

	j = 0;
	for (size_t i = 0; i < N && matchv[i] && matchv[i][0] != '\0'; ++i) {
		const auto id = search_get_tag_id(tabv[i]);
		if (id == -1) {
			screen_status_printf(_("Bad search tag %s"), tabv[i]);
		} else {
			table[j] = id;
			arg[j] = locale_to_utf8(matchv[i]);
			j++;
			advanced_search_mode = true;
		}
	}

	g_free(str);

	if (!advanced_search_mode || j == 0) {
		for (size_t i = 0; arg[i] != nullptr; ++i)
			g_free(arg[i]);
		return nullptr;
	}

	/*-----------------------------------------------------------------------
	 * NOTE (again): This code exists to test a new search ui,
	 *               Its ugly and MUST be redesigned before the next release!
	 *             + the code below should live in mpdclient.c
	 *-----------------------------------------------------------------------
	 */
	/** stupid - but this is just a test...... (fulhack)  */
	mpd_search_db_songs(connection, false);

	for (size_t i = 0; i < N && arg[i] != nullptr; i++) {
		if (table[i] == SEARCH_URI)
			mpd_search_add_uri_constraint(connection,
						      MPD_OPERATOR_DEFAULT,
						      arg[i]);
		else
			mpd_search_add_tag_constraint(connection,
						      MPD_OPERATOR_DEFAULT,
						      (enum mpd_tag_type)table[i], arg[i]);
	}

	mpd_search_commit(connection);
	auto *fl = filelist_new_recv(connection);
	if (!mpd_response_finish(connection)) {
		delete fl;
		fl = nullptr;
	}

	for (size_t i = 0; arg[i] != nullptr; ++i)
		g_free(arg[i]);

	return fl;
}

static FileList *
do_search(struct mpdclient *c, const char *query)
{
	auto *connection = c->GetConnection();
	if (connection == nullptr)
		return nullptr;

	auto *fl = search_advanced_query(connection, query);
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

	lw.hide_cursor = false;
	delete filelist;
	filelist = do_search(&c, pattern.c_str());
	if (filelist == nullptr)
		filelist = new FileList();
	lw.SetLength(filelist->size());

	screen_browser_sync_highlights(filelist, &c.playlist);

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
SearchPage::OnOpen(gcc_unused struct mpdclient &c)
{
	//  if( pattern==nullptr )
	//    search_new(screen, c);
	// else
	screen_status_printf(_("Press %s for a new search"),
			     GetGlobalKeyBindings().GetKeyNames(Command::SCREEN_SEARCH,
								false));
}

void
SearchPage::Paint() const
{
	if (filelist) {
		FileListPage::Paint();
	} else {
		lw.Paint(TextListRenderer(SearchHelpText()));
	}
}

const char *
SearchPage::GetTitle(char *str, size_t size) const
{
	if (advanced_search_mode && !pattern.empty())
		snprintf(str, size, _("Search: %s"), pattern.c_str());
	else if (!pattern.empty())
		snprintf(str, size,
			 _("Search: Results for %s [%s]"),
			 pattern.c_str(),
			 gettext(mode[options.search_mode].label));
	else
		snprintf(str, size, _("Search: Press %s for a new search [%s]"),
			 GetGlobalKeyBindings().GetKeyNames(Command::SCREEN_SEARCH,
							    false),
			 gettext(mode[options.search_mode].label));

	return str;
}

void
SearchPage::Update(struct mpdclient &c, unsigned events)
{
	if (filelist != nullptr && events & MPD_IDLE_QUEUE) {
		screen_browser_sync_highlights(filelist, &c.playlist);
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
		/* fall through */
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
