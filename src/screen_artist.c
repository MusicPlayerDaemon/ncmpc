/*
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

#include "i18n.h"
#include "options.h"
#include "charset.h"
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

#define BUFSIZE 1024

typedef enum { LIST_ARTISTS, LIST_ALBUMS, LIST_SONGS } artist_mode_t;

static artist_mode_t mode = LIST_ARTISTS;
static GPtrArray *artist_list, *album_list;
static char *artist = NULL;
static char *album  = NULL;

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
	GPtrArray *list = data;
	static char buf[BUFSIZE];
	char *str, *str_utf8;

	if (mode == LIST_ALBUMS) {
		if (idx == 0)
			return "[..]";
		else if (idx == list->len + 1) {
			str = utf8_to_locale(_("All tracks"));
			g_snprintf(buf, BUFSIZE, "[%s]", str);
			g_free(str);
			return buf;
		}

		--idx;
	}

	if (idx >= list->len)
		return NULL;

	str_utf8 = g_ptr_array_index(list, idx);
	assert(str_utf8 != NULL);

	str = utf8_to_locale(str_utf8);
	g_snprintf(buf, BUFSIZE, "[%s]", str);
	g_free(str);

	return buf;
}

static void
paint(void);

static void
artist_repaint(void)
{
	paint();
	wrefresh(browser.lw->w);
}

static void
artist_repaint_if_active(void)
{
	if (screen_is_visible(&screen_artist))
		artist_repaint();
}

/* the playlist have been updated -> fix highlights */
static void
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
	browser_playlist_changed(&browser, c, event, data);

	artist_repaint_if_active();
}

static GPtrArray *
g_list_to_ptr_array(GList *in)
{
	GPtrArray *out = g_ptr_array_sized_new(g_list_length(in));
	GList *head = in;

	while (in != NULL) {
		g_ptr_array_add(out, in->data);
		in = g_list_next(in);
	}

	g_list_free(head);
	return out;
}

static void
string_array_free(GPtrArray *array)
{
	unsigned i;

	for (i = 0; i < array->len; ++i) {
		char *value = g_ptr_array_index(array, i);
		free(value);
	}

	g_ptr_array_free(array, TRUE);
}

static void
free_lists(struct mpdclient *c)
{
	if (artist_list != NULL) {
		string_array_free(artist_list);
		artist_list = NULL;
	}

	if (album_list != NULL) {
		string_array_free(album_list);
		album_list = NULL;
	}

	if (browser.filelist) {
		if (c != NULL)
			mpdclient_remove_playlist_callback(c, playlist_changed_callback);
		filelist_free(browser.filelist);
		browser.filelist = NULL;
	}
}

static void
load_artist_list(struct mpdclient *c)
{
	GList *list;

	assert(mode == LIST_ARTISTS);
	assert(artist == NULL);
	assert(album == NULL);
	assert(artist_list == NULL);
	assert(album_list == NULL);
	assert(browser.filelist == NULL);

	list = mpdclient_get_artists(c);
	/* sort list */
	list = g_list_sort(list, compare_utf8);

	artist_list = g_list_to_ptr_array(list);
}

static void
load_album_list(struct mpdclient *c)
{
	GList *list;

	assert(mode == LIST_ALBUMS);
	assert(artist != NULL);
	assert(album == NULL);
	assert(album_list == NULL);
	assert(browser.filelist == NULL);

	list = mpdclient_get_albums(c, artist);
	/* sort list */
	list = g_list_sort(list, compare_utf8);

	album_list = g_list_to_ptr_array(list);
}

static void
load_song_list(struct mpdclient *c)
{
	assert(mode == LIST_SONGS);
	assert(artist != NULL);
	assert(album != NULL);
	assert(browser.filelist == NULL);

	if (album[0] == 0)
		browser.filelist =
			mpdclient_filelist_search(c, TRUE,
						  MPD_TABLE_ARTIST,
						  artist);
	else
		browser.filelist =
			mpdclient_filelist_search(c, TRUE,
						  MPD_TABLE_ALBUM,
						  album);
	if (browser.filelist == NULL)
		browser.filelist = filelist_new(NULL);

	/* add a dummy entry for ".." */
	filelist_prepend(browser.filelist, NULL);

	/* install playlist callback and fix highlights */
	sync_highlights(c, browser.filelist);
	mpdclient_install_playlist_callback(c, playlist_changed_callback);
}

static void
free_state(struct mpdclient *c)
{
	g_free(artist);
	g_free(album);
	artist = NULL;
	album = NULL;

	free_lists(c);
}

static void
open_artist_list(struct mpdclient *c)
{
	free_state(c);

	mode = LIST_ARTISTS;
	load_artist_list(c);
}

static void
open_album_list(struct mpdclient *c, char *_artist)
{
	assert(_artist != NULL);

	free_state(c);

	mode = LIST_ALBUMS;
	artist = _artist;
	load_album_list(c);
}

static void
open_song_list(struct mpdclient *c, char *_artist, char *_album)
{
	assert(_artist != NULL);
	assert(_album != NULL);

	free_state(c);

	mode = LIST_SONGS;
	artist = _artist;
	album = _album;
	load_song_list(c);
}

static void
reload_lists(struct mpdclient *c)
{
	free_lists(c);

	switch (mode) {
	case LIST_ARTISTS:
		load_artist_list(c);
		break;

	case LIST_ALBUMS:
		load_album_list(c);
		break;

	case LIST_SONGS:
		load_song_list(c);
		break;
	}
}

/* db updated */
static void
browse_callback(mpdclient_t *c, int event, mpd_unused gpointer data)
{
	switch(event) {
	case BROWSE_DB_UPDATED:
		reload_lists(c);
		break;
	default:
		break;
	}

	artist_repaint_if_active();
}

static void
init(WINDOW *w, int cols, int rows)
{
	browser.lw = list_window_init(w, cols, rows);
	artist = NULL;
	album = NULL;
}

static void
quit(void)
{
	free_state(NULL);
	list_window_free(browser.lw);
}

static void
open(mpdclient_t *c)
{
	static gboolean callback_installed = FALSE;

	if (artist_list == NULL && album_list == NULL &&
	    browser.filelist == NULL)
		reload_lists(c);
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
paint(void)
{
	if (browser.filelist) {
		list_window_paint(browser.lw, browser_lw_callback,
				  browser.filelist);
	} else if (album_list != NULL)
		list_window_paint(browser.lw, artist_lw_callback, album_list);
	else if (artist_list != NULL)
		list_window_paint(browser.lw, artist_lw_callback, artist_list);
	else {
		wmove(browser.lw->w, 0, 0);
		wclrtobot(browser.lw->w);
	}
}

static const char *
get_title(char *str, size_t size)
{
	char *s1, *s2;

	switch(mode) {
	case LIST_ARTISTS:
		g_snprintf(str, size, _("All artists"));
		break;

	case LIST_ALBUMS:
		s1 = utf8_to_locale(artist);
		g_snprintf(str, size, _("Albums of artist: %s"), s1);
		g_free(s1);
		break;

	case LIST_SONGS:
		s1 = utf8_to_locale(artist);
		if (*album != 0) {
			s2 = utf8_to_locale(album);
			g_snprintf(str, size,
				   _("Album: %s - %s"), s1, s2);
			g_free(s2);
		} else
			g_snprintf(str, size,
				   _("All tracks of artist: %s"), s1);
		g_free(s1);
		break;
	}

	return str;
}

static void
add_query(mpdclient_t *c, int table, char *_filter)
{
	char *str;
	mpdclient_filelist_t *addlist;

	assert(filter != NULL);

	str = utf8_to_locale(_filter);
	if (table== MPD_TABLE_ALBUM)
		screen_status_printf("Adding album %s...", str);
	else
		screen_status_printf("Adding %s...", str);
	g_free(str);

	addlist = mpdclient_filelist_search(c, TRUE, table, _filter);
	if (addlist) {
		mpdclient_filelist_add_all(c, addlist);
		filelist_free(addlist);
	}
}

static unsigned
metalist_length(void)
{
	assert(mode != LIST_ARTISTS || artist_list != NULL);
	assert(mode != LIST_ALBUMS || album_list != NULL);

	return mode == LIST_ALBUMS
		? album_list->len + 2
		: artist_list->len;
}

static int
artist_lw_cmd(struct mpdclient *c, command_t cmd)
{
	switch (mode) {
	case LIST_ARTISTS:
	case LIST_ALBUMS:
		return list_window_cmd(browser.lw, metalist_length(), cmd);

	case LIST_SONGS:
		return browser_cmd(&browser, c, cmd);
	}

	assert(0);
	return 0;
}

static int
string_array_find(GPtrArray *array, const char *value)
{
	guint i;

	for (i = 0; i < array->len; ++i)
		if (strcmp((const char*)g_ptr_array_index(array, i),
			   value) == 0)
			return i;

	return -1;
}

static bool
artist_cmd(mpdclient_t *c, command_t cmd)
{
	char *selected;
	char *old;
	int idx;

	switch(cmd) {
	case CMD_PLAY:
		switch (mode) {
		case LIST_ARTISTS:
			selected = g_ptr_array_index(artist_list,
						     browser.lw->selected);
			open_album_list(c, g_strdup(selected));
			list_window_reset(browser.lw);

			artist_repaint();
			return true;

		case LIST_ALBUMS:
			if (browser.lw->selected == 0) {
				/* handle ".." */
				old = g_strdup(artist);

				open_artist_list(c);
				list_window_reset(browser.lw);
				/* restore previous list window state */
				idx = string_array_find(artist_list, old);
				g_free(old);

				if (idx >= 0) {
					list_window_set_selected(browser.lw, idx);
					list_window_center(browser.lw,
							   artist_list->len, idx);
				}
			} else if (browser.lw->selected == album_list->len + 1) {
				/* handle "show all" */
				open_song_list(c, g_strdup(artist), g_strdup("\0"));
				list_window_reset(browser.lw);
			} else {
				/* select album */
				selected = g_ptr_array_index(album_list,
							     browser.lw->selected - 1);
				open_song_list(c, g_strdup(artist), g_strdup(selected));
				list_window_reset(browser.lw);
			}

			artist_repaint();
			return true;

		case LIST_SONGS:
			if (browser.lw->selected == 0) {
				/* handle ".." */
				old = g_strdup(album);

				open_album_list(c, g_strdup(artist));
				list_window_reset(browser.lw);
				/* restore previous list window state */
				idx = *old == 0
					? (int)album_list->len
					: string_array_find(album_list, old);
				g_free(old);

				if (idx >= 0) {
					++idx;
					list_window_set_selected(browser.lw, idx);
					list_window_center(browser.lw,
							   album_list->len, idx);
				}

				artist_repaint();
				return true;
			}
			break;
		}
		break;

		/* FIXME? CMD_GO_* handling duplicates code from CMD_PLAY */

	case CMD_GO_PARENT_DIRECTORY:
		switch (mode) {
		case LIST_ARTISTS:
			break;

		case LIST_ALBUMS:
			old = g_strdup(artist);

			open_artist_list(c);
			list_window_reset(browser.lw);
			/* restore previous list window state */
			idx = string_array_find(artist_list, old);
			g_free(old);

			if (idx >= 0) {
				list_window_set_selected(browser.lw, idx);
				list_window_center(browser.lw,
						   artist_list->len, idx);
			}
			break;

		case LIST_SONGS:
			old = g_strdup(album);

			open_album_list(c, g_strdup(artist));
			list_window_reset(browser.lw);
			/* restore previous list window state */
			idx = *old == 0
				? (int)album_list->len
				: string_array_find(album_list, old);
			g_free(old);

			if (idx >= 0) {
				++idx;
				list_window_set_selected(browser.lw, idx);
				list_window_center(browser.lw,
						   album_list->len, idx);
			}
			break;
		}

		artist_repaint();
		break;

	case CMD_GO_ROOT_DIRECTORY:
		switch (mode) {
		case LIST_ARTISTS:
			break;

		case LIST_ALBUMS:
		case LIST_SONGS:
			open_artist_list(c);
			list_window_reset(browser.lw);
			/* restore first list window state (pop while returning true) */
			/* XXX */
			break;
		}

		artist_repaint();
		break;

	case CMD_SELECT:
	case CMD_ADD:
		switch(mode) {
		case LIST_ARTISTS:
			selected = g_ptr_array_index(artist_list,
						     browser.lw->selected);
			add_query(c, MPD_TABLE_ARTIST, selected);
			cmd = CMD_LIST_NEXT; /* continue and select next item... */
			break;

		case LIST_ALBUMS:
			if (browser.lw->selected == album_list->len + 1)
				add_query(c, MPD_TABLE_ARTIST, artist);
			else if (browser.lw->selected > 0) {
				selected = g_ptr_array_index(album_list,
							     browser.lw->selected - 1);
				add_query(c, MPD_TABLE_ALBUM, selected);
				cmd = CMD_LIST_NEXT; /* continue and select next item... */
			}
			break;

		case LIST_SONGS:
			/* handled by browser_cmd() */
			break;
		}
		break;

		/* continue and update... */
	case CMD_SCREEN_UPDATE:
		reload_lists(c);
		screen_status_printf(_("Screen updated!"));
		return false;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		switch (mode) {
		case LIST_ARTISTS:
			screen_find(browser.lw, artist_list->len,
				    cmd, artist_lw_callback, artist_list);
			artist_repaint();
			return true;

		case LIST_ALBUMS:
			screen_find(browser.lw, album_list->len + 2,
				    cmd, artist_lw_callback, album_list);
			artist_repaint();
			return true;

		case LIST_SONGS:
			/* handled by browser_cmd() */
			break;
		}

		break;

	default:
		break;
	}

	if (artist_lw_cmd(c, cmd)) {
		if (screen_is_visible(&screen_artist))
			artist_repaint();
		return true;
	}

	return false;
}

const struct screen_functions screen_artist = {
	.init = init,
	.exit = quit,
	.open = open,
	.resize = resize,
	.paint = paint,
	.cmd = artist_cmd,
	.get_title = get_title,
};
