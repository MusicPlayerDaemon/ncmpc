/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "i18n.h"
#include "options.h"
#include "charset.h"
#include "mpdclient.h"
#include "strfsong.h"
#include "command.h"
#include "screen.h"
#include "utils.h"
#include "screen_utils.h"
#include "screen_browser.h"
#include "filelist.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

enum {
	SEARCH_URI = MPD_TAG_COUNT + 100,
};

static const struct {
	const char *name;
	const char *localname;
} search_tag[MPD_TAG_COUNT] = {
	[MPD_TAG_ARTIST] = { "artist", N_("artist") },
	[MPD_TAG_ALBUM] = { "album", N_("album") },
	[MPD_TAG_TITLE] = { "title", N_("title") },
	[MPD_TAG_TRACK] = { "track", N_("track") },
	[MPD_TAG_NAME] = { "name", N_("name") },
	[MPD_TAG_GENRE] = { "genre", N_("genre") },
	[MPD_TAG_DATE] = { "date", N_("date") },
	[MPD_TAG_COMPOSER] = { "composer", N_("composer") },
	[MPD_TAG_PERFORMER] = { "performer", N_("performer") },
	[MPD_TAG_COMMENT] = { "comment", N_("comment") },
};

static int
search_get_tag_id(const char *name)
{
	unsigned i;

	if (g_ascii_strcasecmp(name, "file") == 0 ||
	    strcasecmp(name, _("file")) == 0)
		return SEARCH_URI;

	for (i = 0; i < MPD_TAG_COUNT; ++i)
		if (search_tag[i].name != NULL &&
		    (strcasecmp(search_tag[i].name, name) == 0 ||
		     strcasecmp(search_tag[i].localname, name) == 0))
			return i;

	return -1;
}

#define SEARCH_ARTIST_TITLE 999

typedef struct {
	enum mpd_tag_type table;
	const char *label;
} search_type_t;

static search_type_t mode[] = {
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ SEARCH_URI, N_("file") },
	{ SEARCH_ARTIST_TITLE, N_("Artist + Title") },
	{ 0, NULL }
};

static GList *search_history = NULL;
static gchar *pattern = NULL;
static gboolean advanced_search_mode = FALSE;

static struct screen_browser browser;


/* search info */
static const char *
lw_search_help_callback(unsigned idx, G_GNUC_UNUSED bool *highlight,
			G_GNUC_UNUSED char** sc, G_GNUC_UNUSED void *data)
{
	unsigned text_rows;
	static const char *text[] = {
		"Quick     -  Enter a string and ncmpc will search according",
		"             to the current search mode (displayed above).",
		"",
		"Advanced  -  <tag>:<search term> [<tag>:<search term>...]",
		"		Example: artist:radiohead album:pablo honey",
		"",
		"		Available tags: artist, album, title, track,",
		"		name, genre, date composer, performer, comment, file",
		"",
		NULL
	};

	text_rows=0;
	while (text[text_rows])
		text_rows++;

	if (idx < text_rows)
		return text[idx];
	return NULL;
}

static void
paint(void);

static void
search_repaint(void)
{
	paint();
	wrefresh(browser.lw->w);
}

/* sanity check search mode value */
static void
search_check_mode(void)
{
	int max = 0;

	while (mode[max].label != NULL)
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
	}
	if (clear_pattern && pattern) {
		g_free(pattern);
		pattern = NULL;
	}
}

static struct filelist *
filelist_search(struct mpdclient *c, G_GNUC_UNUSED int exact_match, int table,
		gchar *local_pattern)
{
	struct filelist *list, *list2;
	gchar *filter_utf8 = locale_to_utf8(local_pattern);

	if (table == SEARCH_ARTIST_TITLE) {
		list = mpdclient_filelist_search(c, FALSE, MPD_TAG_ARTIST,
						 filter_utf8);
		if (list == NULL)
			list = filelist_new();

		list2 = mpdclient_filelist_search(c, FALSE, MPD_TAG_TITLE,
						  filter_utf8);
		if (list2 != NULL) {
			filelist_move(list, list2);
			filelist_free(list2);
		}

		filelist_sort_all(list, compare_filelistentry_format);
	} else {
		list = mpdclient_filelist_search(c, FALSE, table, filter_utf8);
		if (list == NULL)
			list = filelist_new();
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
search_advanced_query(char *query, struct mpdclient *c)
{
	int i,j;
	char **strv;
	int table[10];
	char *arg[10];
	struct filelist *fl = NULL;

	advanced_search_mode = FALSE;
	if( g_strrstr(query, ":") == NULL )
		return NULL;

	strv = g_strsplit_set(query, ": ", 0);

	i=0;
	while (strv[i]) {
		i++;
	}

	memset(table, 0, 10*sizeof(int));
	memset(arg, 0, 10*sizeof(char *));

	i=0;
	j=0;
	while (strv[i] && strlen(strv[i]) > 0 && i < 9) {
		int id = search_get_tag_id(strv[i]);
		if (id == -1) {
			if (table[j]) {
				char *tmp = arg[j];
				arg[j] = g_strdup_printf("%s %s", arg[j], strv[i]);
				g_free(tmp);
			} else {
				screen_status_printf(_("Bad search tag %s"), strv[i]);
			}
			i++;
		} else if (strv[i+1] == NULL || strlen(strv[i+1]) == 0) {
			screen_status_printf(_("No argument for search tag %s"), strv[i]);
			i++;
			//	  j--;
			//table[j] = -1;
		} else {
			table[j] = id;
			arg[j] = locale_to_utf8(strv[i+1]); // FREE ME
			j++;
			table[j] = -1;
			arg[j] = NULL;
			i = i + 2;
			advanced_search_mode = TRUE;
		}
	}

	g_strfreev(strv);


	if (advanced_search_mode && j > 0) {
		int iter;
		struct mpd_entity *entity;

		/*-----------------------------------------------------------------------
		 * NOTE (again): This code exists to test a new search ui,
		 *               Its ugly and MUST be redesigned before the next release!
		 *             + the code below should live in mpdclient.c
		 *-----------------------------------------------------------------------
		 */
		/** stupid - but this is just a test...... (fulhack)  */
		mpd_search_db_songs(c->connection, false);

		for(iter = 0; iter < 10 && arg[iter] != NULL; iter++) {
			if (table[iter] == SEARCH_URI)
				mpd_search_add_uri_constraint(c->connection,
							      MPD_OPERATOR_DEFAULT,
							      arg[iter]);
			else
				mpd_search_add_tag_constraint(c->connection,
							      MPD_OPERATOR_DEFAULT,
							      table[iter], arg[iter]);
		}

		mpd_search_commit(c->connection);

		fl = filelist_new();

		while ((entity = mpd_recv_entity(c->connection)) != NULL)
			filelist_append(fl, entity);

		if (!mpd_response_finish(c->connection)) {
			filelist_free(fl);
			fl = NULL;

			mpdclient_handle_error(c);
		}
	}

	i=0;
	while( arg[i] )
		g_free(arg[i++]);

	return fl;
}

static void
search_new(struct mpdclient *c)
{
	if (c->connection == NULL)
		return;

	search_clear(true);

	g_free(pattern);
	pattern = screen_readln(_("Search"),
				NULL,
				&search_history,
				NULL);

	if (pattern == NULL) {
		list_window_reset(browser.lw);
		return;
	}

	if (browser.filelist != NULL) {
		filelist_free(browser.filelist);
		browser.filelist = NULL;
	}

	if (mpd_connection_cmp_server_version(c->connection, 0, 12, 0) >= 0)
		browser.filelist = search_advanced_query(pattern, c);

	if (!advanced_search_mode && browser.filelist == NULL)
		browser.filelist = filelist_search(c, FALSE,
						  mode[options.search_mode].table,
						  pattern);

	if (browser.filelist == NULL)
		browser.filelist = filelist_new();

	sync_highlights(c, browser.filelist);
	list_window_check_selected(browser.lw, filelist_length(browser.filelist));
}



static void
init(WINDOW *w, int cols, int rows)
{
	browser.lw = list_window_init(w, cols, rows);
}

static void
quit(void)
{
	if (search_history)
		string_list_free(search_history);
	if (browser.filelist)
		filelist_free(browser.filelist);
	list_window_free(browser.lw);

	if (pattern) {
		g_free(pattern);
		pattern = NULL;
	}
}

static void
open(G_GNUC_UNUSED struct mpdclient *c)
{
	//  if( pattern==NULL )
	//    search_new(screen, c);
	// else
	screen_status_printf(_("Press %s for a new search"),
			     get_key_names(CMD_SCREEN_SEARCH,0));
	search_check_mode();
}

static void
resize(int cols, int rows)
{
	browser.lw->cols = cols;
	browser.lw->rows = rows;
}

static void
paint(void)
{
	if (browser.filelist) {
		browser.lw->hide_cursor = false;
		list_window_paint(browser.lw, browser_lw_callback, browser.filelist);
	} else {
		browser.lw->hide_cursor = true;
		list_window_paint(browser.lw, lw_search_help_callback, NULL);
	}
}

static const char *
get_title(char *str, size_t size)
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
			   get_key_names(CMD_SCREEN_SEARCH,0),
			   _(mode[options.search_mode].label));

	return str;
}

static void
screen_search_update(struct mpdclient *c)
{
	if (browser.filelist != NULL && c->events & MPD_IDLE_PLAYLIST) {
		sync_highlights(c, browser.filelist);
		search_repaint();
	}
}

static bool
search_cmd(struct mpdclient *c, command_t cmd)
{
	switch (cmd) {
	case CMD_SEARCH_MODE:
		options.search_mode++;
		if (mode[options.search_mode].label == NULL)
			options.search_mode = 0;
		screen_status_printf(_("Search mode: %s"),
				     _(mode[options.search_mode].label));
		/* continue and update... */
	case CMD_SCREEN_UPDATE:
		if (pattern) {
			search_clear(false);
			browser.filelist = filelist_search(c,
							  FALSE,
							  mode[options.search_mode].table,
							  pattern);
			sync_highlights(c, browser.filelist);
		}
		search_repaint();
		return true;

	case CMD_SCREEN_SEARCH:
		search_new(c);
		search_repaint();
		return true;

	case CMD_CLEAR:
		search_clear(true);
		list_window_reset(browser.lw);
		search_repaint();
		return true;

	default:
		break;
	}

	if (browser.filelist != NULL &&
	    browser_cmd(&browser, c, cmd)) {
		if (screen_is_visible(&screen_search))
			search_repaint();
		return true;
	}

	return false;
}

const struct screen_functions screen_search = {
	.init = init,
	.exit = quit,
	.open = open,
	.resize = resize,
	.paint = paint,
	.update = screen_search_update,
	.cmd = search_cmd,
	.get_title = get_title,
};
