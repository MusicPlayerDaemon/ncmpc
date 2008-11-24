/*
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

static const struct {
	const char *name;
	const char *localname;
} search_tag[MPD_TAG_NUM_OF_ITEM_TYPES] = {
	[MPD_TAG_ITEM_ARTIST] = { "artist", N_("artist") },
	[MPD_TAG_ITEM_ALBUM] = { "album", N_("album") },
	[MPD_TAG_ITEM_TITLE] = { "title", N_("title") },
	[MPD_TAG_ITEM_TRACK] = { "track", N_("track") },
	[MPD_TAG_ITEM_NAME] = { "name", N_("name") },
	[MPD_TAG_ITEM_GENRE] = { "genre", N_("genre") },
	[MPD_TAG_ITEM_DATE] = { "date", N_("date") },
	[MPD_TAG_ITEM_COMPOSER] = { "composer", N_("composer") },
	[MPD_TAG_ITEM_PERFORMER] = { "performer", N_("performer") },
	[MPD_TAG_ITEM_COMMENT] = { "comment", N_("comment") },
	[MPD_TAG_ITEM_FILENAME] = { "filename", N_("file") },
};

static int
search_get_tag_id(char *name)
{
	unsigned i;

	for (i = 0; i < MPD_TAG_NUM_OF_ITEM_TYPES; ++i)
		if (search_tag[i].name != NULL &&
		    (strcasecmp(search_tag[i].name, name) == 0 ||
		     strcasecmp(search_tag[i].localname, name) == 0))
			return i;

	return -1;
}

#define SEARCH_TITLE    0
#define SEARCH_ARTIST   1
#define SEARCH_ALBUM    2
#define SEARCH_FILE     3

#define SEARCH_ARTIST_TITLE 999

typedef struct {
	int table;
	const char *label;
} search_type_t;

static search_type_t mode[] = {
	{ MPD_TABLE_TITLE, N_("Title") },
	{ MPD_TABLE_ARTIST, N_("Artist") },
	{ MPD_TABLE_ALBUM, N_("Album") },
	{ MPD_TABLE_FILENAME, N_("Filename") },
	{ SEARCH_ARTIST_TITLE, N_("Artist + Title") },
	{ 0, NULL }
};

static GList *search_history = NULL;
static gchar *pattern = NULL;
static gboolean advanced_search_mode = FALSE;

static struct screen_browser browser;


/* search info */
static const char *
lw_search_help_callback(unsigned idx, G_GNUC_UNUSED int *highlight,
			G_GNUC_UNUSED void *data)
{
	unsigned text_rows;
	static const char *text[] = {
		"Quick  - just enter a string and ncmpc will search according",
		"               to the current search mode (displayed above).",
		"",
		"Advanced  -  <tag>:<search term> [<tag>:<search term>...]",
		"			Example: artist:radiohead album:pablo honey",
		"",
		"		   avalible tags: artist, album, title, track,",
		"	           name, genre, date composer, performer, comment, file",
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

static void
search_repaint_if_active(void)
{
	if (screen_is_visible(&screen_search))
		search_repaint();
}

/* the playlist have been updated -> fix highlights */
static void
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
	browser_playlist_changed(&browser, c, event, data);
	search_repaint_if_active();
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
search_clear(mpdclient_t *c,
	     gboolean clear_pattern)
{
	if (browser.filelist) {
		mpdclient_remove_playlist_callback(c, playlist_changed_callback);
		filelist_free(browser.filelist);
		browser.filelist = filelist_new(NULL);
	}
	if (clear_pattern && pattern) {
		g_free(pattern);
		pattern = NULL;
	}
}

static mpdclient_filelist_t *
filelist_search(mpdclient_t *c, G_GNUC_UNUSED int exact_match, int table,
		gchar *local_pattern)
{
	mpdclient_filelist_t *list, *list2;
	gchar *filter_utf8 = locale_to_utf8(local_pattern);

	if (table == SEARCH_ARTIST_TITLE) {
		list = mpdclient_filelist_search(c, FALSE, MPD_TABLE_ARTIST,
						 filter_utf8);
		if (list == NULL)
			list = filelist_new(NULL);

		list2 = mpdclient_filelist_search(c, FALSE, MPD_TABLE_TITLE,
						  filter_utf8);
		if (list2 != NULL) {
			filelist_move(list, list2);
			filelist_free(list2);
		}

		filelist_sort(list, compare_filelistentry_format);
	} else {
		list = mpdclient_filelist_search(c, FALSE, table, filter_utf8);
		if (list == NULL)
			list = filelist_new(NULL);
	}

	g_free(filter_utf8);
	return list;
}

/*-----------------------------------------------------------------------
 * NOTE: This code exists to test a new search ui,
 *       Its ugly and MUST be redesigned before the next release!
 *-----------------------------------------------------------------------
 */
static mpdclient_filelist_t *
search_advanced_query(char *query, mpdclient_t *c)
{
	int i,j;
	char **strv;
	int table[10];
	char *arg[10];
	mpdclient_filelist_t *fl = NULL;

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
		mpd_InfoEntity *entity;

		/*-----------------------------------------------------------------------
		 * NOTE (again): This code exists to test a new search ui,
		 *               Its ugly and MUST be redesigned before the next release!
		 *             + the code below should live in mpdclient.c
		 *-----------------------------------------------------------------------
		 */
		/** stupid - but this is just a test...... (fulhack)  */
		mpd_startSearch(c->connection, FALSE);

		for(iter = 0; iter < 10; iter++) {
			mpd_addConstraintSearch(c->connection, table[iter], arg[iter]);
		}

		mpd_commitSearch(c->connection);

		fl = g_malloc0(sizeof(mpdclient_filelist_t));

		while ((entity=mpd_getNextInfoEntity(c->connection)))
			filelist_append(fl, entity);

		if (mpdclient_finish_command(c) && fl)
			filelist_free(fl);
	}

	i=0;
	while( arg[i] )
		g_free(arg[i++]);

	return fl;
}

static void
search_new(mpdclient_t *c)
{
	search_clear(c, TRUE);

	g_free(pattern);
	pattern = screen_readln(screen.status_window.w,
				_("Search: "),
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

	if (!MPD_VERSION_LT(c, 0, 12, 0))
		browser.filelist = search_advanced_query(pattern, c);

	if (!advanced_search_mode && browser.filelist == NULL)
		browser.filelist = filelist_search(c, FALSE,
						  mode[options.search_mode].table,
						  pattern);

	if (browser.filelist == NULL)
		browser.filelist = filelist_new(NULL);

	sync_highlights(c, browser.filelist);
	mpdclient_install_playlist_callback(c, playlist_changed_callback);
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
open(G_GNUC_UNUSED mpdclient_t *c)
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
		browser.lw->flags = 0;
		list_window_paint(browser.lw, browser_lw_callback, browser.filelist);
	} else {
		browser.lw->flags = LW_HIDE_CURSOR;
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

static bool
search_cmd(mpdclient_t *c, command_t cmd)
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
			search_clear(c, FALSE);
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
		search_clear(c, TRUE);
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
	.cmd = search_cmd,
	.get_title = get_title,
};
