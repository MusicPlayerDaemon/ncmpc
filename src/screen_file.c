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

#include "screen_file.h"
#include "screen_browser.h"
#include "screen_interface.h"
#include "screen_message.h"
#include "screen.h"
#include "config.h"
#include "i18n.h"
#include "charset.h"
#include "mpdclient.h"
#include "filelist.h"
#include "screen_utils.h"
#include "screen_play.h"
#include "screen_client.h"

#include <mpd/client.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

static struct screen_browser browser;
static char *current_path;

static void
screen_file_paint(void);

static void
screen_file_repaint(void)
{
	screen_file_paint();
	wrefresh(browser.lw->w);
}

static void
screen_file_reload(struct mpdclient *c)
{
	struct mpd_connection *connection;

	if (browser.filelist != NULL)
		filelist_free(browser.filelist);

	browser.filelist = filelist_new();
	if (*current_path != 0)
		/* add a dummy entry for ./.. */
		filelist_append(browser.filelist, NULL);

	if (!mpdclient_is_connected(c))
		return;

	connection = mpdclient_get_connection(c);

	mpd_send_list_meta(connection, current_path);
	filelist_recv(browser.filelist, connection);

	if (mpd_response_finish(connection))
		filelist_sort_dir_play(browser.filelist,
				       compare_filelist_entry_path);
	else
		mpdclient_handle_error(c);

	list_window_set_length(browser.lw,
			       filelist_length(browser.filelist));
}

/**
 * Change to the specified absolute directory.
 */
static bool
change_directory(struct mpdclient *c, const char *new_path)
{
	g_free(current_path);
	current_path = g_strdup(new_path);

	screen_file_reload(c);

#ifndef NCMPC_MINI
	screen_browser_sync_highlights(browser.filelist, &c->playlist);
#endif

	list_window_reset(browser.lw);

	return browser.filelist != NULL;
}

/**
 * Change to the parent directory of the current directory.
 */
static bool
change_to_parent(struct mpdclient *c)
{
	char *parent = g_path_get_dirname(current_path);
	char *old_path;
	int idx;
	bool success;

	if (strcmp(parent, ".") == 0)
		parent[0] = '\0';

	old_path = current_path;
	current_path = NULL;

	success = change_directory(c, parent);
	g_free(parent);

	idx = success
		? filelist_find_directory(browser.filelist, old_path)
		: -1;
	g_free(old_path);

	if (success && idx >= 0) {
		/* set the cursor on the previous working directory */
		list_window_set_cursor(browser.lw, idx);
		list_window_center(browser.lw, idx);
	}

	return success;
}

/**
 * Change to the directory referred by the specified #filelist_entry
 * object.
 */
static bool
change_to_entry(struct mpdclient *c, const struct filelist_entry *entry)
{
	assert(entry != NULL);

	if (entry->entity == NULL)
		return change_to_parent(c);
	else if (mpd_entity_get_type(entry->entity) == MPD_ENTITY_TYPE_DIRECTORY)
		return change_directory(c, mpd_directory_get_path(mpd_entity_get_directory(entry->entity)));
	else
		return false;
}

static bool
screen_file_handle_enter(struct mpdclient *c)
{
	const struct filelist_entry *entry = browser_get_selected_entry(&browser);

	if (entry == NULL)
		return false;

	return change_to_entry(c, entry);
}

static int
handle_save(struct mpdclient *c)
{
	struct list_window_range range;
	const char *defaultname = NULL;
	char *defaultname_utf8 = NULL;
	int ret;

	list_window_get_range(browser.lw, &range);
	if (range.start == range.end)
		return -1;

	for (unsigned i = range.start; i < range.end; ++i) {
		struct filelist_entry *entry =
			filelist_get(browser.filelist, i);
		if( entry && entry->entity ) {
			struct mpd_entity *entity = entry->entity;
			if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
				const struct mpd_playlist *playlist =
					mpd_entity_get_playlist(entity);
				defaultname = mpd_playlist_get_path(playlist);
			}
		}
	}

	if(defaultname)
		defaultname_utf8 = utf8_to_locale(defaultname);
	ret = playlist_save(c, NULL, defaultname_utf8);
	g_free(defaultname_utf8);

	return ret;
}

static int
handle_delete(struct mpdclient *c)
{
	struct mpd_connection *connection = mpdclient_get_connection(c);
	struct list_window_range range;
	struct mpd_entity *entity;
	const struct mpd_playlist *playlist;
	char *str, *buf;
	int key;

	list_window_get_range(browser.lw, &range);
	for (unsigned i = range.start; i < range.end; ++i) {
		struct filelist_entry *entry =
			filelist_get(browser.filelist, i);
		if( entry==NULL || entry->entity==NULL )
			continue;

		entity = entry->entity;

		if (mpd_entity_get_type(entity) != MPD_ENTITY_TYPE_PLAYLIST) {
			/* translators: the "delete" command is only possible
			   for playlists; the user attempted to delete a song
			   or a directory or something else */
			screen_status_printf(_("Deleting this item is not possible"));
			screen_bell();
			continue;
		}

		playlist = mpd_entity_get_playlist(entity);
		str = utf8_to_locale(g_basename(mpd_playlist_get_path(playlist)));
		buf = g_strdup_printf(_("Delete playlist %s [%s/%s] ? "), str, YES, NO);
		g_free(str);
		key = tolower(screen_getch(buf));
		g_free(buf);
		if( key != YES[0] ) {
			/* translators: a dialog was aborted by the user */
			screen_status_printf(_("Aborted"));
			return 0;
		}

		if (!mpd_run_rm(connection, mpd_playlist_get_path(playlist))) {
			mpdclient_handle_error(c);
			break;
		}

		c->events |= MPD_IDLE_STORED_PLAYLIST;

		/* translators: MPD deleted the playlist, as requested by the
		   user */
		screen_status_printf(_("Playlist deleted"));
	}
	return 0;
}

static void
screen_file_init(WINDOW *w, int cols, int rows)
{
	current_path = g_strdup("");

	browser.lw = list_window_init(w, cols, rows);
}

static void
screen_file_resize(int cols, int rows)
{
	list_window_resize(browser.lw, cols, rows);
}

static void
screen_file_exit(void)
{
	if (browser.filelist)
		filelist_free(browser.filelist);
	list_window_free(browser.lw);

	g_free(current_path);
}

static void
screen_file_open(struct mpdclient *c)
{
	screen_file_reload(c);
}

static const char *
screen_file_get_title(char *str, size_t size)
{
	const char *path = NULL, *prev = NULL, *slash = current_path;
	char *path_locale;

	/* determine the last 2 parts of the path */
	while ((slash = strchr(slash, '/')) != NULL) {
		path = prev;
		prev = ++slash;
	}

	if (path == NULL)
		/* fall back to full path */
		path = current_path;

	path_locale = utf8_to_locale(path);
	g_snprintf(str, size, "%s: %s",
		   /* translators: caption of the browser screen */
		   _("Browse"), path_locale);
	g_free(path_locale);
	return str;
}

static void
screen_file_paint(void)
{
	list_window_paint(browser.lw, browser_lw_callback, browser.filelist);
}

static void
screen_file_update(struct mpdclient *c)
{
	if (c->events & (MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST)) {
		/* the db has changed -> update the filelist */
		screen_file_reload(c);
	}

#ifndef NCMPC_MINI
	if (c->events & (MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST |
			 MPD_IDLE_PLAYLIST))
		screen_browser_sync_highlights(browser.filelist, &c->playlist);
#endif

	if (c->events & (MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST
#ifndef NCMPC_MINI
			 | MPD_IDLE_PLAYLIST
#endif
			 ))
		screen_file_repaint();
}

static bool
screen_file_cmd(struct mpdclient *c, command_t cmd)
{
	switch(cmd) {
	case CMD_PLAY:
		if (screen_file_handle_enter(c)) {
			screen_file_repaint();
			return true;
		}

		break;

	case CMD_GO_ROOT_DIRECTORY:
		change_directory(c, "");
		screen_file_repaint();
		return true;
	case CMD_GO_PARENT_DIRECTORY:
		change_to_parent(c);
		screen_file_repaint();
		return true;

	case CMD_LOCATE:
		/* don't let browser_cmd() evaluate the locate command
		   - it's a no-op, and by the way, leads to a
		   segmentation fault in the current implementation */
		return false;

	case CMD_SCREEN_UPDATE:
		screen_file_reload(c);
#ifndef NCMPC_MINI
		screen_browser_sync_highlights(browser.filelist, &c->playlist);
#endif
		screen_file_repaint();
		return false;

	default:
		break;
	}

	if (browser_cmd(&browser, c, cmd)) {
		if (screen_is_visible(&screen_browse))
			screen_file_repaint();
		return true;
	}

	if (!mpdclient_is_connected(c))
		return false;

	switch(cmd) {
	case CMD_DELETE:
		handle_delete(c);
		screen_file_repaint();
		break;

	case CMD_SAVE_PLAYLIST:
		handle_save(c);
		break;

	case CMD_DB_UPDATE:
		screen_database_update(c, current_path);
		return true;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_browse = {
	.init = screen_file_init,
	.exit = screen_file_exit,
	.open = screen_file_open,
	.resize = screen_file_resize,
	.paint = screen_file_paint,
	.update = screen_file_update,
	.cmd = screen_file_cmd,
	.get_title = screen_file_get_title,
};

bool
screen_file_goto_song(struct mpdclient *c, const struct mpd_song *song)
{
	const char *uri, *slash, *parent;
	char *allocated = NULL;
	bool ret;
	int i;

	assert(song != NULL);

	uri = mpd_song_get_uri(song);

	if (strstr(uri, "//") != NULL)
		/* an URL? */
		return false;

	/* determine the song's parent directory and go there */

	slash = strrchr(uri, '/');
	if (slash != NULL)
		parent = allocated = g_strndup(uri, slash - uri);
	else
		parent = "";

	ret = change_directory(c, parent);
	g_free(allocated);
	if (!ret)
		return false;

	/* select the specified song */

	i = filelist_find_song(browser.filelist, song);
	if (i < 0)
		i = 0;

	list_window_set_cursor(browser.lw, i);

	/* finally, switch to the file screen */
	screen_switch(&screen_browse, c);
	return true;
}
