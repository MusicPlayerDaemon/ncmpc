/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "screen_search.hxx"
#include "screen_interface.hxx"
#include "screen_status.hxx"
#include "screen.hxx"
#include "i18n.h"
#include "options.hxx"
#include "charset.hxx"
#include "mpdclient.hxx"
#include "strfsong.hxx"
#include "utils.hxx"
#include "screen_utils.hxx"
#include "screen_browser.hxx"
#include "filelist.hxx"

#include <glib.h>

#include <string.h>

enum {
	SEARCH_URI = MPD_TAG_COUNT + 100,
	SEARCH_ARTIST_TITLE
};

static const struct {
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
	if (g_ascii_strcasecmp(name, "file") == 0 ||
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

static search_type_t mode[] = {
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ (enum mpd_tag_type)SEARCH_URI, N_("Filename") },
	{ (enum mpd_tag_type)SEARCH_ARTIST_TITLE, N_("Artist + Title") },
	{ MPD_TAG_COUNT, nullptr }
};

static GList *search_history = nullptr;
static gchar *pattern = nullptr;
static bool advanced_search_mode = false;

static struct screen_browser browser;

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

/* search info */
static const char *
lw_search_help_callback(unsigned idx, gcc_unused void *data)
{
	assert(idx < G_N_ELEMENTS(help_text));

	return help_text[idx];
}

/* sanity check search mode value */
static void
search_check_mode()
{
	int max = 0;

	while (mode[max].label != nullptr)
		max++;
	if (options.search_mode < 0)
		options.search_mode = 0;
	else if (options.search_mode >= max)
		options.search_mode = max-1;
}

static void
search_clear(bool clear_pattern)
{
	if (browser.filelist) {
		filelist_free(browser.filelist);
		browser.filelist = filelist_new();
		list_window_set_length(browser.lw, 0);
	}
	if (clear_pattern && pattern) {
		g_free(pattern);
		pattern = nullptr;
	}
}

static struct filelist *
search_simple_query(struct mpd_connection *connection, bool exact_match,
		    int table, gchar *local_pattern)
{
	struct filelist *list;
	gchar *filter_utf8 = locale_to_utf8(local_pattern);

	if (table == SEARCH_ARTIST_TITLE) {
		mpd_command_list_begin(connection, false);

		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_ARTIST, filter_utf8);
		mpd_search_commit(connection);

		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      MPD_TAG_TITLE, filter_utf8);
		mpd_search_commit(connection);

		mpd_command_list_end(connection);

		list = filelist_new_recv(connection);
		filelist_no_duplicates(list);
	} else if (table == SEARCH_URI) {
		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_uri_constraint(connection, MPD_OPERATOR_DEFAULT,
					      filter_utf8);
		mpd_search_commit(connection);

		list = filelist_new_recv(connection);
	} else {
		mpd_search_db_songs(connection, exact_match);
		mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT,
					      (enum mpd_tag_type)table, filter_utf8);
		mpd_search_commit(connection);

		list = filelist_new_recv(connection);
	}

	g_free(filter_utf8);
	return list;
}

/*-----------------------------------------------------------------------
 * NOTE: This code exists to test a new search ui,
 *       Its ugly and MUST be redesigned before the next release!
 *-----------------------------------------------------------------------
 */
static struct filelist *
search_advanced_query(struct mpd_connection *connection, char *query)
{
	advanced_search_mode = false;
	if (strchr(query, ':') == nullptr)
		return nullptr;

	int i, j;
	char *str = g_strdup(query);

	char *tabv[10];
	char *matchv[10];
	int table[10];
	char *arg[10];

	memset(tabv, 0, 10 * sizeof(char *));
	memset(matchv, 0, 10 * sizeof(char *));
	memset(arg, 0, 10 * sizeof(char *));

	for (i = 0; i < 10; i++)
		table[i] = -1;

	/*
	 * Replace every : with a '\0' and every space character
	 * before it unless spi = -1, link the resulting strings
	 * to their proper vector.
	 */
	int spi = -1;
	j = 0;
	for (i = 0; str[i] != '\0' && j < 10; i++) {
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

	int id = j = i = 0;
	while (matchv[i] && matchv[i][0] != '\0' && i < 10) {
		id = search_get_tag_id(tabv[i]);
		if (id == -1) {
			screen_status_printf(_("Bad search tag %s"), tabv[i]);
		} else {
			table[j] = id;
			arg[j] = locale_to_utf8(matchv[i]);
			j++;
			advanced_search_mode = true;
		}

		i++;
	}

	g_free(str);

	if (!advanced_search_mode || j == 0) {
		for (i = 0; arg[i] != nullptr; ++i)
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

	for (i = 0; i < 10 && arg[i] != nullptr; i++) {
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
	struct filelist *fl = filelist_new_recv(connection);
	if (!mpd_response_finish(connection)) {
		filelist_free(fl);
		fl = nullptr;
	}

	for (i = 0; arg[i] != nullptr; ++i)
		g_free(arg[i]);

	return fl;
}

static struct filelist *
do_search(struct mpdclient *c, char *query)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr)
		return nullptr;

	struct filelist *fl = search_advanced_query(connection, query);
	if (fl != nullptr)
		return fl;

	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		mpdclient_handle_error(c);
		return nullptr;
	}

	fl = search_simple_query(connection, false,
				 mode[options.search_mode].table,
				 query);
	if (fl == nullptr)
		mpdclient_handle_error(c);
	return fl;
}

static void
screen_search_reload(struct mpdclient *c)
{
	if (pattern == nullptr)
		return;

	if (browser.filelist != nullptr) {
		filelist_free(browser.filelist);
		browser.filelist = nullptr;
	}

	browser.filelist = do_search(c, pattern);
	if (browser.filelist == nullptr)
		browser.filelist = filelist_new();
	list_window_set_length(browser.lw, filelist_length(browser.filelist));

	screen_browser_sync_highlights(browser.filelist, &c->playlist);
}

static void
search_new(struct mpdclient *c)
{
	if (!mpdclient_is_connected(c))
		return;

	search_clear(true);

	g_free(pattern);
	pattern = screen_readln(_("Search"),
				nullptr,
				&search_history,
				nullptr);

	if (pattern == nullptr) {
		list_window_reset(browser.lw);
		return;
	}

	screen_search_reload(c);
}

static void
screen_search_init(WINDOW *w, unsigned cols, unsigned rows)
{
	browser.lw = list_window_init(w, cols, rows);
	if (options.search_format != nullptr) {
		browser.song_format = options.search_format;
	} else {
		browser.song_format = options.list_format;
	}
	list_window_set_length(browser.lw, G_N_ELEMENTS(help_text));
}

static void
screen_search_quit()
{
	if (search_history)
		string_list_free(search_history);
	if (browser.filelist)
		filelist_free(browser.filelist);
	list_window_free(browser.lw);

	if (pattern) {
		g_free(pattern);
		pattern = nullptr;
	}
}

static void
screen_search_open(gcc_unused struct mpdclient *c)
{
	//  if( pattern==nullptr )
	//    search_new(screen, c);
	// else
	screen_status_printf(_("Press %s for a new search"),
			     get_key_names(CMD_SCREEN_SEARCH, false));
	search_check_mode();
}

static void
screen_search_resize(unsigned cols, unsigned rows)
{
	list_window_resize(browser.lw, cols, rows);
}

static void
screen_search_paint()
{
	if (browser.filelist) {
		browser.lw->hide_cursor = false;
		screen_browser_paint(&browser);
	} else {
		browser.lw->hide_cursor = true;
		list_window_paint(browser.lw, lw_search_help_callback, nullptr);
	}
}

static const char *
screen_search_get_title(char *str, size_t size)
{
	if (advanced_search_mode && pattern)
		g_snprintf(str, size, _("Search: %s"), pattern);
	else if (pattern)
		g_snprintf(str, size,
			   _("Search: Results for %s [%s]"),
			   pattern,
			   _(mode[options.search_mode].label));
	else
		g_snprintf(str, size, _("Search: Press %s for a new search [%s]"),
			   get_key_names(CMD_SCREEN_SEARCH, false),
			   _(mode[options.search_mode].label));

	return str;
}

static void
screen_search_update(struct mpdclient *c)
{
	if (browser.filelist != nullptr && c->events & MPD_IDLE_QUEUE) {
		screen_browser_sync_highlights(browser.filelist, &c->playlist);
		screen_search_paint();
	}
}

static bool
screen_search_cmd(struct mpdclient *c, command_t cmd)
{
	switch (cmd) {
	case CMD_SEARCH_MODE:
		options.search_mode++;
		if (mode[options.search_mode].label == nullptr)
			options.search_mode = 0;
		screen_status_printf(_("Search mode: %s"),
				     _(mode[options.search_mode].label));
		/* fall through */
	case CMD_SCREEN_UPDATE:
		screen_search_reload(c);
		screen_search_paint();
		return true;

	case CMD_SCREEN_SEARCH:
		search_new(c);
		screen_search_paint();
		return true;

	case CMD_CLEAR:
		search_clear(true);
		list_window_reset(browser.lw);
		screen_search_paint();
		return true;

	default:
		break;
	}

	if (browser.filelist != nullptr &&
	    browser_cmd(&browser, c, cmd)) {
		if (screen_is_visible(&screen_search))
			screen_search_paint();
		return true;
	}

	return false;
}

#ifdef HAVE_GETMOUSE
static bool
screen_search_mouse(struct mpdclient *c, int x, int y, mmask_t bstate)
{
	if (browser_mouse(&browser, c, x, y, bstate)) {
		if (screen_is_visible(&screen_search))
			screen_search_paint();

		return true;
	}

	return false;
}
#endif

const struct screen_functions screen_search = {
	.init = screen_search_init,
	.exit = screen_search_quit,
	.open = screen_search_open,
	.close = nullptr,
	.resize = screen_search_resize,
	.paint = screen_search_paint,
	.update = screen_search_update,
	.cmd = screen_search_cmd,
#ifdef HAVE_GETMOUSE
	.mouse = screen_search_mouse,
#endif
	.get_title = screen_search_get_title,
};
