/* 
 * $Id$
 *
 * (c) 2005 by Kalle Wallin <kaw@linux.se>
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

#include "config.h"

#ifndef DISABLE_ARTIST_SCREEN
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "mpdclient.h"
#include "utils.h"
#include "strfsong.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_browser.h"
#include "gcc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#define BUFSIZE 1024

typedef enum { LIST_ARTISTS, LIST_ALBUMS, LIST_SONGS } artist_mode_t;

static artist_mode_t mode = LIST_ARTISTS;
static char *artist = NULL;
static char *album  = NULL;
static unsigned metalist_length = 0;
static GList *metalist = NULL;

static struct screen_browser browser;

static gint
compare_utf8(gconstpointer s1, gconstpointer s2)
{
	char *key1, *key2;
	int n;

	key1 = g_utf8_collate_key(s1,-1);
	key2 = g_utf8_collate_key(s2,-1);
	n = strcmp(key1,key2);
	g_free(key1);
	g_free(key2);
	return n;
}

/* list_window callback */
static const char *
artist_lw_callback(unsigned idx, mpd_unused int *highlight, mpd_unused void *data)
{
	static char buf[BUFSIZE];
	char *str, *str_utf8;

	if ((str_utf8 = (char *)g_list_nth_data(metalist, idx)) == NULL)
		return NULL;

	str = utf8_to_locale(str_utf8);
	g_snprintf(buf, BUFSIZE, "[%s]", str);
	g_free(str);

	return buf;
}

/* the playlist have been updated -> fix highlights */
static void
playlist_changed_callback(mpdclient_t *c, int event, mpd_unused gpointer data)
{
	if (browser.filelist == NULL)
		return;

	D("screen_artist.c> playlist_callback() [%d]\n", event);
	switch(event) {
	case PLAYLIST_EVENT_CLEAR:
		clear_highlights(browser.filelist);
		break;
	default:
		sync_highlights(c, browser.filelist);
		break;
	}
}

/* fetch artists/albums/songs from mpd */
static void
update_metalist(mpdclient_t *c, char *m_artist, char *m_album)
{
	g_free(artist);
	g_free(album);
	artist = NULL;
	album = NULL;

	if (metalist)
		metalist = string_list_free(metalist);
	if (browser.filelist) {
		mpdclient_remove_playlist_callback(c, playlist_changed_callback);
		mpdclient_filelist_free(browser.filelist);
		browser.filelist = NULL;
	}

	if (m_album) {
		/* retreive songs... */
		filelist_entry_t *entry;

		artist = m_artist;
		album = m_album;
		if (album[0] == 0) {
			album = g_strdup(_("All tracks"));
			browser.filelist =
				mpdclient_filelist_search_utf8(c, TRUE,
							       MPD_TABLE_ARTIST,
							       artist);
		} else
			browser.filelist =
				mpdclient_filelist_search_utf8(c, TRUE,
							       MPD_TABLE_ALBUM,
							       album);
		/* add a dummy entry for ".." */
		entry = g_malloc0(sizeof(filelist_entry_t));
		entry->entity = NULL;
		browser.filelist->list = g_list_insert(browser.filelist->list,
						      entry, 0);
		browser.filelist->length++;
		/* install playlist callback and fix highlights */
		sync_highlights(c, browser.filelist);
		mpdclient_install_playlist_callback(c, playlist_changed_callback);
		mode = LIST_SONGS;
	} else if (m_artist) {
		/* retreive albums... */

		artist = m_artist;
		metalist = mpdclient_get_albums_utf8(c, m_artist);
		/* sort list */
		metalist = g_list_sort(metalist, compare_utf8);
		/* add a dummy entry for ".." */
		metalist = g_list_insert(metalist, g_strdup(".."), 0);
		/* add a dummy entry for all songs */
		metalist = g_list_insert(metalist, g_strdup(_("All tracks")), -1);
		mode = LIST_ALBUMS;
	} else {
		/* retreive artists... */

		metalist = mpdclient_get_artists_utf8(c);
		/* sort list */
		metalist = g_list_sort(metalist, compare_utf8);
		mode = LIST_ARTISTS;
	}
	metalist_length = g_list_length(metalist);
	browser.lw->clear = TRUE;
}

/* db updated */
static void
browse_callback(mpdclient_t *c, int event, mpd_unused gpointer data)
{
	switch(event) {
	case BROWSE_DB_UPDATED:
		D("screen_artist.c> browse_callback() [BROWSE_DB_UPDATED]\n");
		browser.lw->clear = 1;
		browser.lw->repaint = 1;
		update_metalist(c, g_strdup(artist), g_strdup(album));
		break;
	default:
		break;
	}
}

static void
init(WINDOW *w, int cols, int rows)
{
	browser.lw = list_window_init(w, cols, rows);
	browser.lw_state = list_window_init_state();
	artist = NULL;
	album = NULL;
}

static void
quit(void)
{
	if (browser.filelist)
		mpdclient_filelist_free(browser.filelist);
	if (metalist)
		string_list_free(metalist);
	g_free(artist);
	g_free(album);
	artist = NULL;
	album = NULL;
	list_window_free(browser.lw);
	list_window_free_state(browser.lw_state);
}

static void
open(mpd_unused screen_t *screen, mpdclient_t *c)
{
	static gboolean callback_installed = FALSE;

	if (metalist == NULL && browser.filelist == NULL)
		update_metalist(c, NULL, NULL);
	if (!callback_installed) {
		mpdclient_install_browse_callback(c, browse_callback);
		callback_installed = TRUE;
	}
}

static void
resize(int cols, int rows)
{
	browser.lw->cols = cols;
	browser.lw->rows = rows;
}

static void
paint(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	browser.lw->clear = 1;

	if (browser.filelist) {
		list_window_paint(browser.lw, browser_lw_callback,
				  browser.filelist);
		browser.filelist->updated = FALSE;
	} else if (metalist) {
		list_window_paint(browser.lw, artist_lw_callback, metalist);
	} else {
		wmove(browser.lw->w, 0, 0);
		wclrtobot(browser.lw->w);
	}

	wnoutrefresh(browser.lw->w);
}

static void
update(screen_t *screen, mpdclient_t *c)
{
	if (browser.filelist && !browser.filelist->updated)
		list_window_paint(browser.lw, browser_lw_callback,
				  browser.filelist);
	else if (metalist)
		list_window_paint(browser.lw, artist_lw_callback, metalist);
	else
		paint(screen, c);
	wnoutrefresh(browser.lw->w);
}

static const char *
get_title(char *str, size_t size)
{
	char *s1 = artist ? utf8_to_locale(artist) : NULL;
	char *s2 = album ? utf8_to_locale(album) : NULL;

	switch(mode) {
	case LIST_ARTISTS:
		g_snprintf(str, size,  _("Artist: [db browser - EXPERIMENTAL]"));
		break;
	case LIST_ALBUMS:
		g_snprintf(str, size,  _("Artist: %s"), s1);
		break;
	case LIST_SONGS:
		g_snprintf(str, size,  _("Artist: %s - %s"), s1, s2);
		break;
	}
	g_free(s1);
	g_free(s2);
	return str;
}

static void
add_query(mpdclient_t *c, int table, char *_filter)
{
	char *str;
	mpdclient_filelist_t *addlist;

	str = utf8_to_locale(_filter);
	if (table== MPD_TABLE_ALBUM)
		screen_status_printf("Adding album %s...", str);
	else
		screen_status_printf("Adding %s...", str);
	g_free(str);

	addlist = mpdclient_filelist_search_utf8(c, TRUE, table, _filter);
	if (addlist) {
		mpdclient_filelist_add_all(c, addlist);
		mpdclient_filelist_free(addlist);
	}
}

static int
artist_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
	char *selected;

	switch(cmd) {
	case CMD_PLAY:
		switch (mode) {
		case LIST_ARTISTS:
			selected = (char *) g_list_nth_data(metalist,
							    browser.lw->selected);
			update_metalist(c, g_strdup(selected), NULL);
			list_window_push_state(browser.lw_state, browser.lw);
			break;

		case LIST_ALBUMS:
			if (browser.lw->selected == 0) {
				/* handle ".." */

				update_metalist(c, NULL, NULL);
				list_window_reset(browser.lw);
				/* restore previous list window state */
				list_window_pop_state(browser.lw_state, browser.lw);
			} else if (browser.lw->selected == metalist_length - 1) {
				/* handle "show all" */
				update_metalist(c, g_strdup(artist), g_strdup("\0"));
				list_window_push_state(browser.lw_state, browser.lw);
			} else {
				/* select album */
				selected = g_list_nth_data(metalist,
							   browser.lw->selected);
				update_metalist(c, g_strdup(artist), g_strdup(selected));
				list_window_push_state(browser.lw_state, browser.lw);
			}
			break;

		case LIST_SONGS:
			if (browser.lw->selected == 0) {
				/* handle ".." */

				update_metalist(c, g_strdup(artist), NULL);
				list_window_reset(browser.lw);
				/* restore previous list window state */
				list_window_pop_state(browser.lw_state,
						      browser.lw);
			} else
				browser_handle_enter(&browser, c);
			break;
		}
		return 1;


		/* FIXME? CMD_GO_* handling duplicates code from CMD_PLAY */

	case CMD_GO_PARENT_DIRECTORY:
		switch (mode) {
		case LIST_ARTISTS:
			break;

		case LIST_ALBUMS:
			update_metalist(c, NULL, NULL);
			list_window_reset(browser.lw);
			/* restore previous list window state */
			list_window_pop_state(browser.lw_state, browser.lw);
			break;

		case LIST_SONGS:
			update_metalist(c, g_strdup(artist), NULL);
			list_window_reset(browser.lw);
			/* restore previous list window state */
			list_window_pop_state(browser.lw_state, browser.lw);
			break;
		}
		break;

	case CMD_GO_ROOT_DIRECTORY:
		switch (mode) {
		case LIST_ARTISTS:
			break;

		case LIST_ALBUMS:
		case LIST_SONGS:
			update_metalist(c, NULL, NULL);
			list_window_reset(browser.lw);
			/* restore first list window state (pop while returning true) */
			while(list_window_pop_state(browser.lw_state, browser.lw));
			break;
		}
		break;

	case CMD_SELECT:
		switch(mode) {
		case LIST_ARTISTS:
			selected = g_list_nth_data(metalist,
						   browser.lw->selected);
			add_query(c, MPD_TABLE_ARTIST, selected);
			cmd = CMD_LIST_NEXT; /* continue and select next item... */
			break;

		case LIST_ALBUMS:
			if (browser.lw->selected &&
			    browser.lw->selected == metalist_length - 1)
				add_query(c, MPD_TABLE_ARTIST, artist);
			else if (browser.lw->selected > 0) {
				selected = g_list_nth_data(metalist,
							   browser.lw->selected);
				add_query(c, MPD_TABLE_ALBUM, selected);
				cmd = CMD_LIST_NEXT; /* continue and select next item... */
			}
			break;

		case LIST_SONGS:
			if (browser_handle_select(&browser, c) == 0)
				/* continue and select next item... */
				cmd = CMD_LIST_NEXT;
			break;
		}
		break;

		/* continue and update... */
	case CMD_SCREEN_UPDATE:
		screen->painted = 0;
		browser.lw->clear = 1;
		browser.lw->repaint = 1;
		update_metalist(c, g_strdup(artist), g_strdup(album));
		screen_status_printf(_("Screen updated!"));
		return 0;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		if (browser.filelist)
			return screen_find(screen,
					   browser.lw, browser.filelist->length,
					   cmd, browser_lw_callback,
					   browser.filelist);
		else if (metalist)
			return screen_find(screen,
					   browser.lw, metalist_length,
					   cmd, artist_lw_callback, metalist);
		else
			return 1;

	case CMD_MOUSE_EVENT:
		return browser_handle_mouse_event(&browser, c);

	default:
		break;
	}

	if (browser.filelist)
		return list_window_cmd(browser.lw, browser.filelist->length, cmd);
	else if (metalist)
		return list_window_cmd(browser.lw, metalist_length, cmd);

	return 0;
}

const struct screen_functions screen_artist = {
	.init = init,
	.exit = quit,
	.open = open,
	.resize = resize,
	.paint = paint,
	.update = update,
	.cmd = artist_cmd,
	.get_title = get_title,
};

#endif /* ENABLE_ARTIST_SCREEN */
