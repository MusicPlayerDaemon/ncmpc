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

#include "config.h"
#include "i18n.h"
#include "options.h"
#include "charset.h"
#include "mpdclient.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_browser.h"
#include "screen_play.h"
#include "gcc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

static struct screen_browser browser;

static void
browse_paint(mpdclient_t *c);

static void
file_repaint(void)
{
	browse_paint(NULL);
	wrefresh(browser.lw->w);
}

static void
file_repaint_if_active(void)
{
	if (screen_is_visible(&screen_browse))
		file_repaint();
}

/* the db have changed -> update the filelist */
static void
file_changed_callback(mpdclient_t *c, mpd_unused int event,
		      mpd_unused gpointer data)
{
	browser.filelist = mpdclient_filelist_update(c, browser.filelist);
	sync_highlights(c, browser.filelist);
	list_window_check_selected(browser.lw, filelist_length(browser.filelist));

	file_repaint_if_active();
}

/* the playlist have been updated -> fix highlights */
static void
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
	browser_playlist_changed(&browser, c, event, data);

	file_repaint_if_active();
}

static int
handle_save(screen_t *screen, mpdclient_t *c)
{
	filelist_entry_t *entry;
	char *defaultname = NULL;

	if (browser.lw->selected >= filelist_length(browser.filelist))
		return -1;

	entry = filelist_get(browser.filelist, browser.lw->selected);
	if( entry && entry->entity ) {
		mpd_InfoEntity *entity = entry->entity;
		if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE ) {
			mpd_PlaylistFile *plf = entity->info.playlistFile;
			defaultname = plf->path;
		}
	}

	return playlist_save(screen, c, NULL, defaultname);
}

static int
handle_delete(screen_t *screen, mpdclient_t *c)
{
	filelist_entry_t *entry;
	mpd_InfoEntity *entity;
	mpd_PlaylistFile *plf;
	char *str, *buf;
	int key;

	if (browser.lw->selected >= filelist_length(browser.filelist))
		return -1;

	entry = filelist_get(browser.filelist, browser.lw->selected);
	if( entry==NULL || entry->entity==NULL )
		return -1;

	entity = entry->entity;

	if( entity->type!=MPD_INFO_ENTITY_TYPE_PLAYLISTFILE ) {
		screen_status_printf(_("You can only delete playlists!"));
		screen_bell();
		return -1;
	}

	plf = entity->info.playlistFile;
	str = utf8_to_locale(g_basename(plf->path));
	buf = g_strdup_printf(_("Delete playlist %s [%s/%s] ? "), str, YES, NO);
	g_free(str);
	key = tolower(screen_getch(screen->status_window.w, buf));
	g_free(buf);
	if( key != YES[0] ) {
		screen_status_printf(_("Aborted!"));
		return 0;
	}

	if( mpdclient_cmd_delete_playlist_utf8(c, plf->path) )
		return -1;

	screen_status_printf(_("Playlist deleted!"));
	return 0;
}

static void
browse_init(WINDOW *w, int cols, int rows)
{
	browser.lw = list_window_init(w, cols, rows);
}

static void
browse_resize(int cols, int rows)
{
	browser.lw->cols = cols;
	browser.lw->rows = rows;
}

static void
browse_exit(void)
{
	if (browser.filelist)
		filelist_free(browser.filelist);
	list_window_free(browser.lw);
}

static void
browse_open(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	if (browser.filelist == NULL) {
		browser.filelist = mpdclient_filelist_get(c, "");
		mpdclient_install_playlist_callback(c, playlist_changed_callback);
		mpdclient_install_browse_callback(c, file_changed_callback);
	}
}

static const char *
browse_title(char *str, size_t size)
{
	char *dirname, *parentdir;

	dirname = g_path_get_dirname(browser.filelist->path);
	parentdir = g_path_get_basename(dirname);

	if( parentdir[0] == '.' && strlen(parentdir) == 1 ) {
		parentdir = NULL;
	}

	g_snprintf(str, size, _("Browse: %s%s%s"),
		   parentdir ? parentdir : "",
		   parentdir ? "/" : "",
		   g_basename(browser.filelist->path));
	free(dirname);
	free(parentdir);
	return str;
}

static void
browse_paint(mpd_unused mpdclient_t *c)
{
	list_window_paint(browser.lw, browser_lw_callback, browser.filelist);
}

static int
browse_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
	switch(cmd) {
	case CMD_GO_ROOT_DIRECTORY:
		browser_change_directory(&browser, c, NULL, "");
		file_repaint();
		return 1;
	case CMD_GO_PARENT_DIRECTORY:
		browser_change_directory(&browser, c, NULL, "..");
		file_repaint();
		return 1;

	case CMD_DELETE:
		handle_delete(screen, c);
		file_repaint();
		break;
	case CMD_SAVE_PLAYLIST:
		handle_save(screen, c);
		break;
	case CMD_SCREEN_UPDATE:
		browser.filelist = mpdclient_filelist_update(c, browser.filelist);
		sync_highlights(c, browser.filelist);
		list_window_check_selected(browser.lw,
					   filelist_length(browser.filelist));
		file_repaint();

		screen_status_printf(_("Screen updated!"));
		return 0;

	case CMD_DB_UPDATE:
		if (c->status == NULL)
			return 1;

		if (!c->status->updatingDb) {
			if (mpdclient_cmd_db_update_utf8(c, browser.filelist->path) == 0) {
				if (strcmp(browser.filelist->path, ""))
					screen_status_printf(_("Database update of %s started!"),
							     browser.filelist->path);
				else
					screen_status_printf(_("Database update started!"));

				/* set updatingDb to make shure the browse callback gets called
				 * even if the updated has finished before status is updated */
				c->status->updatingDb = 1;
			}
		} else
			screen_status_printf(_("Database update running..."));
		return 1;

	default:
		break;
	}

	if (browser_cmd(&browser, screen, c, cmd)) {
		file_repaint();
		return 1;
	}

	return 0;
}

const struct screen_functions screen_browse = {
	.init = browse_init,
	.exit = browse_exit,
	.open = browse_open,
	.resize = browse_resize,
	.paint = browse_paint,
	.cmd = browse_cmd,
	.get_title = browse_title,
};

