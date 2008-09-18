/* 
 * $Id$
 *
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
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "mpdclient.h"
#include "strfsong.h"
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

#define USE_OLD_LAYOUT
#undef  USE_OLD_ADD

#define BUFSIZE 1024

#define HIGHLIGHT  (0x01)

static struct screen_browser browser;

/* clear the highlight flag for all items in the filelist */
void
clear_highlights(mpdclient_filelist_t *fl)
{
	GList *list = g_list_first(fl->list);

	while( list ) {
		filelist_entry_t *entry = list->data;

		entry->flags &= ~HIGHLIGHT;
		list = list->next;
	}
}

/* change the highlight flag for a song */
void
set_highlight(mpdclient_filelist_t *fl, mpd_Song *song, int highlight)
{
	GList *list = g_list_first(fl->list);

	if( !song )
		return;

	while( list ) {
		filelist_entry_t *entry = list->data;
		mpd_InfoEntity *entity  = entry->entity;

		if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
			mpd_Song *song2 = entity->info.song;

			if( strcmp(song->file, song2->file) == 0 ) {
				if(highlight)
					entry->flags |= HIGHLIGHT;
				else
					entry->flags &= ~HIGHLIGHT;
			}
		}
		list = list->next;
	}
}

/* sync highlight flags with playlist */
void
sync_highlights(mpdclient_t *c, mpdclient_filelist_t *fl)
{
	GList *list = g_list_first(fl->list);

	while(list) {
		filelist_entry_t *entry = list->data;
		mpd_InfoEntity *entity = entry->entity;

		if( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
			mpd_Song *song = entity->info.song;

			if( playlist_get_index_from_file(c, song->file) >= 0 )
				entry->flags |= HIGHLIGHT;
			else
				entry->flags &= ~HIGHLIGHT;
		}
		list=list->next;
	}
}

/* the db have changed -> update the filelist */
static void
file_changed_callback(mpdclient_t *c, mpd_unused int event,
		      mpd_unused gpointer data)
{
	D("screen_file.c> filelist_callback() [%d]\n", event);
	browser.filelist = mpdclient_filelist_update(c, browser.filelist);
	sync_highlights(c, browser.filelist);
	list_window_check_selected(browser.lw, browser.filelist->length);
}

/* the playlist have been updated -> fix highlights */
static void
playlist_changed_callback(mpdclient_t *c, int event, gpointer data)
{
	D("screen_file.c> playlist_callback() [%d]\n", event);
	switch(event) {
	case PLAYLIST_EVENT_CLEAR:
		clear_highlights(browser.filelist);
		break;
	case PLAYLIST_EVENT_ADD:
		set_highlight(browser.filelist, (mpd_Song *) data, 1);
		break;
	case PLAYLIST_EVENT_DELETE:
		set_highlight(browser.filelist, (mpd_Song *) data, 0);
		break;
	case PLAYLIST_EVENT_MOVE:
		break;
	default:
		sync_highlights(c, browser.filelist);
		break;
	}
}

/* list_window callback */
const char *
browse_lw_callback(unsigned idx, int *highlight, void *data)
{
	static char buf[BUFSIZE];
	mpdclient_filelist_t *fl = (mpdclient_filelist_t *) data;
	filelist_entry_t *entry;
	mpd_InfoEntity *entity;

	if( (entry=(filelist_entry_t *)g_list_nth_data(fl->list,idx))==NULL )
		return NULL;

	entity = entry->entity;
	*highlight = (entry->flags & HIGHLIGHT);

	if( entity == NULL )
		return "[..]";

	if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) {
		mpd_Directory *dir = entity->info.directory;
		char *directory = utf8_to_locale(basename(dir->path));

		g_snprintf(buf, BUFSIZE, "[%s]", directory);
		g_free(directory);
		return buf;
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
		mpd_Song *song = entity->info.song;

		strfsong(buf, BUFSIZE, LIST_FORMAT, song);
		return buf;
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE ) {
		mpd_PlaylistFile *plf = entity->info.playlistFile;
		char *filename = utf8_to_locale(basename(plf->path));

#ifdef USE_OLD_LAYOUT
		g_snprintf(buf, BUFSIZE, "*%s*", filename);
#else
		g_snprintf(buf, BUFSIZE, "<Playlist> %s", filename);
#endif
		g_free(filename);
		return buf;
	}

	return "Error: Unknown entry!";
}

/* chdir */
static int
change_directory(mpd_unused screen_t *screen, mpdclient_t *c,
		 filelist_entry_t *entry, const char *new_path)
{
	mpd_InfoEntity *entity = NULL;
	gchar *path = NULL;

	if( entry!=NULL )
		entity = entry->entity;
	else if( new_path==NULL )
		return -1;

	if( entity==NULL ) {
		if( entry || 0==strcmp(new_path, "..") ) {
			/* return to parent */
			char *parent = g_path_get_dirname(browser.filelist->path);
			if( strcmp(parent, ".") == 0 )
				parent[0] = '\0';
			path = g_strdup(parent);
			list_window_reset(browser.lw);
			/* restore previous list window state */
			list_window_pop_state(browser.lw_state,browser.lw);
		} else {
			/* entry==NULL, then new_path ("" is root) */
			path = g_strdup(new_path);
			list_window_reset(browser.lw);
			/* restore first list window state (pop while returning true) */
			while(list_window_pop_state(browser.lw_state,browser.lw));
		}
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		/* enter sub */
		mpd_Directory *dir = entity->info.directory;
		path = utf8_to_locale(dir->path);
		/* save current list window state */
		list_window_push_state(browser.lw_state,browser.lw);
	} else
		return -1;

	mpdclient_filelist_free(browser.filelist);
	browser.filelist = mpdclient_filelist_get(c, path);
	sync_highlights(c, browser.filelist);
	list_window_check_selected(browser.lw, browser.filelist->length);
	g_free(path);
	return 0;
}

static int
load_playlist(mpd_unused screen_t *screen, mpdclient_t *c,
	      filelist_entry_t *entry)
{
	mpd_InfoEntity *entity = entry->entity;
	mpd_PlaylistFile *plf = entity->info.playlistFile;
	char *filename = utf8_to_locale(plf->path);

	if( mpdclient_cmd_load_playlist_utf8(c, plf->path) == 0 )
		screen_status_printf(_("Loading playlist %s..."), basename(filename));
	g_free(filename);
	return 0;
}

static int
handle_save(screen_t *screen, mpdclient_t *c)
{
	filelist_entry_t *entry;
	char *defaultname = NULL;

	entry = g_list_nth_data(browser.filelist->list, browser.lw->selected);
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

	entry = g_list_nth_data(browser.filelist->list,browser. lw->selected);
	if( entry==NULL || entry->entity==NULL )
		return -1;

	entity = entry->entity;

	if( entity->type!=MPD_INFO_ENTITY_TYPE_PLAYLISTFILE ) {
		screen_status_printf(_("You can only delete playlists!"));
		screen_bell();
		return -1;
	}

	plf = entity->info.playlistFile;
	str = utf8_to_locale(basename(plf->path));
	buf = g_strdup_printf(_("Delete playlist %s [%s/%s] ? "), str, YES, NO);
	g_free(str);
	key = tolower(screen_getch(screen->status_window.w, buf));
	g_free(buf);
	if( key==KEY_RESIZE )
		screen_resize();
	if( key != YES[0] ) {
		screen_status_printf(_("Aborted!"));
		return 0;
	}

	if( mpdclient_cmd_delete_playlist_utf8(c, plf->path) )
		return -1;

	screen_status_printf(_("Playlist deleted!"));
	return 0;
}

static int
enqueue_and_play(mpd_unused screen_t *screen, mpdclient_t *c,
		 filelist_entry_t *entry)
{
	int idx;
	mpd_InfoEntity *entity = entry->entity;
	mpd_Song *song = entity->info.song;

	if(!( entry->flags & HIGHLIGHT )) {
		if( mpdclient_cmd_add(c, song) == 0 ) {
			char buf[BUFSIZE];

			entry->flags |= HIGHLIGHT;
			strfsong(buf, BUFSIZE, LIST_FORMAT, song);
			screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
			mpdclient_update(c); /* get song id */
		} else
			return -1;
	}

	idx = playlist_get_index_from_file(c, song->file);
	mpdclient_cmd_play(c, idx);
	return 0;
}

int
browse_handle_enter(screen_t *screen,
		    mpdclient_t *c,
		    list_window_t *local_lw,
		    mpdclient_filelist_t *fl)
{
	filelist_entry_t *entry;
	mpd_InfoEntity *entity;

	if ( fl==NULL )
		return -1;
	entry = ( filelist_entry_t *) g_list_nth_data(fl->list, local_lw->selected);
	if( entry==NULL )
		return -1;

	entity = entry->entity;
	if( entity==NULL || entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY )
		return change_directory(screen, c, entry, NULL);
	else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
		return load_playlist(screen, c, entry);
	else if( entity->type==MPD_INFO_ENTITY_TYPE_SONG )
		return enqueue_and_play(screen, c, entry);
	return -1;
}


#ifdef USE_OLD_ADD
/* NOTE - The add_directory functions should move to mpdclient.c */
extern gint mpdclient_finish_command(mpdclient_t *c);

static int
add_directory(mpdclient_t *c, char *dir)
{
	mpd_InfoEntity *entity;
	GList *subdir_list = NULL;
	GList *list = NULL;
	char *dirname;

	dirname = utf8_to_locale(dir);
	screen_status_printf(_("Adding directory %s...\n"), dirname);
	doupdate();
	g_free(dirname);
	dirname = NULL;

	mpd_sendLsInfoCommand(c->connection, dir);
	mpd_sendCommandListBegin(c->connection);
	while( (entity=mpd_getNextInfoEntity(c->connection)) ) {
		if( entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
			mpd_Song *song = entity->info.song;
			mpd_sendAddCommand(c->connection, song->file);
			mpd_freeInfoEntity(entity);
		} else if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) {
			subdir_list = g_list_append(subdir_list, (gpointer) entity);
		} else
			mpd_freeInfoEntity(entity);
	}
	mpd_sendCommandListEnd(c->connection);
	mpdclient_finish_command(c);
	c->need_update = TRUE;

	list = g_list_first(subdir_list);
	while( list!=NULL ) {
		mpd_Directory *dir;

		entity = list->data;
		dir = entity->info.directory;
		add_directory(c, dir->path);
		mpd_freeInfoEntity(entity);
		list->data=NULL;
		list=list->next;
	}
	g_list_free(subdir_list);
	return 0;
}
#endif

int
browse_handle_select(screen_t *screen,
		     mpdclient_t *c,
		     list_window_t *local_lw,
		     mpdclient_filelist_t *fl)
{
	filelist_entry_t *entry;

	if ( fl==NULL )
		return -1;
	entry=( filelist_entry_t *) g_list_nth_data(fl->list,
						    local_lw->selected);
	if( entry==NULL || entry->entity==NULL)
		return -1;

	if( entry->entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
		return load_playlist(screen, c, entry);

	if( entry->entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) {
		mpd_Directory *dir = entry->entity->info.directory;
#ifdef USE_OLD_ADD
		add_directory(c, tmp);
#else
		if( mpdclient_cmd_add_path_utf8(c, dir->path) == 0 ) {
			char *tmp = utf8_to_locale(dir->path);

			screen_status_printf(_("Adding \'%s\' to playlist\n"), tmp);
			g_free(tmp);
		}
#endif
		return 0;
	}

	if( entry->entity->type!=MPD_INFO_ENTITY_TYPE_SONG )
		return -1;

	if( entry->flags & HIGHLIGHT )
		entry->flags &= ~HIGHLIGHT;
	else
		entry->flags |= HIGHLIGHT;

	if( entry->flags & HIGHLIGHT ) {
		if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
			mpd_Song *song = entry->entity->info.song;

			if( mpdclient_cmd_add(c, song) == 0 ) {
				char buf[BUFSIZE];

				strfsong(buf, BUFSIZE, LIST_FORMAT, song);
				screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
			}
		}
	} else {
		/* remove song from playlist */
		if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
			mpd_Song *song = entry->entity->info.song;

			if( song ) {
				int idx;

				while( (idx=playlist_get_index_from_file(c, song->file))>=0 )
					mpdclient_cmd_delete(c, idx);
			}
		}
	}
	return 0;
}

int
browse_handle_select_all (screen_t *screen,
			  mpdclient_t *c,
			  mpd_unused list_window_t *local_lw,
			  mpdclient_filelist_t *fl)
{
	filelist_entry_t *entry;
	GList *temp = fl->list;

	if ( fl==NULL )
		return -1;
	for (fl->list = g_list_first(fl->list);
	     fl->list;
	     fl->list = g_list_next(fl->list)) {
		entry=( filelist_entry_t *) fl->list->data;
		if( entry==NULL || entry->entity==NULL)
			return -1;

		if( entry->entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE )
			load_playlist(screen, c, entry);

		if( entry->entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) {
			mpd_Directory *dir = entry->entity->info.directory;
#ifdef USE_OLD_ADD
			add_directory(c, tmp);
#else
			if (mpdclient_cmd_add_path_utf8(c, dir->path) == 0) {
				char *tmp = utf8_to_locale(dir->path);

				screen_status_printf(_("Adding \'%s\' to playlist\n"), tmp);
				g_free(tmp);
			}
#endif
		}

		if( entry->entity->type!=MPD_INFO_ENTITY_TYPE_SONG )
			continue;

		entry->flags |= HIGHLIGHT;

		if( entry->flags & HIGHLIGHT ) {
			if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
				mpd_Song *song = entry->entity->info.song;

				if( mpdclient_cmd_add(c, song) == 0 ) {
					char buf[BUFSIZE];

					strfsong(buf, BUFSIZE, LIST_FORMAT, song);
					screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
				}
			}
		}
		/*
		else {
			//remove song from playlist
			if( entry->entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
				mpd_Song *song = entry->entity->info.song;

				if( song ) {
					int idx = playlist_get_index_from_file(c, song->file);

					while( (idx=playlist_get_index_from_file(c, song->file))>=0 )
						mpdclient_cmd_delete(c, idx);
				}
			}
		}
		*/
		return 0;
	}

	fl->list = temp;
	return 0;
}

static void
browse_init(WINDOW *w, int cols, int rows)
{
	browser.lw = list_window_init(w, cols, rows);
	browser.lw_state = list_window_init_state();
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
		mpdclient_filelist_free(browser.filelist);
	list_window_free(browser.lw);
	list_window_free_state(browser.lw_state);
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
	char *pathcopy;
	char *parentdir;

	pathcopy = strdup(browser.filelist->path);
	parentdir = dirname(pathcopy);
	parentdir = basename(parentdir);

	if( parentdir[0] == '.' && strlen(parentdir) == 1 ) {
		parentdir = NULL;
	}

	g_snprintf(str, size, _("Browse: %s%s%s"),
		   parentdir ? parentdir : "",
		   parentdir ? "/" : "",
		   basename(browser.filelist->path));
	free(pathcopy);
	return str;
}

static void
browse_paint(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	browser.lw->clear = 1;

	list_window_paint(browser.lw, browse_lw_callback, browser.filelist);
	wnoutrefresh(browser.lw->w);
}

static void
browse_update(screen_t *screen, mpdclient_t *c)
{
	if (browser.filelist->updated) {
		browse_paint(screen, c);
		browser.filelist->updated = FALSE;
		return;
	}

	list_window_paint(browser.lw, browse_lw_callback, browser.filelist);
	wnoutrefresh(browser.lw->w);
}


#ifdef HAVE_GETMOUSE
int
browse_handle_mouse_event(screen_t *screen,
			  mpdclient_t *c,
			  list_window_t *local_lw,
			  mpdclient_filelist_t *fl)
{
	int row;
	unsigned prev_selected = local_lw->selected;
	unsigned long bstate;
	int length;

	if ( fl )
		length = fl->length;
	else
		length = 0;

	if( screen_get_mouse_event(c, local_lw, length, &bstate, &row) )
		return 1;

	local_lw->selected = local_lw->start+row;
	list_window_check_selected(local_lw, length);

	if( bstate & BUTTON1_CLICKED ) {
		if( prev_selected == local_lw->selected )
			browse_handle_enter(screen, c, local_lw, fl);
	} else if( bstate & BUTTON3_CLICKED ) {
		if( prev_selected == local_lw->selected )
			browse_handle_select(screen, c, local_lw, fl);
	}

	return 1;
}
#endif

static int
browse_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
	switch(cmd) {
	case CMD_PLAY:
		browse_handle_enter(screen, c, browser.lw,
				    browser.filelist);
		return 1;
	case CMD_GO_ROOT_DIRECTORY:
		return change_directory(screen, c, NULL, "");
		break;
	case CMD_GO_PARENT_DIRECTORY:
		return change_directory(screen, c, NULL, "..");
		break;
	case CMD_SELECT:
		if (browse_handle_select(screen, c, browser.lw,
					 browser.filelist) == 0) {
			/* continue and select next item... */
			cmd = CMD_LIST_NEXT;
		}
		break;
	case CMD_DELETE:
		handle_delete(screen, c);
		break;
	case CMD_SAVE_PLAYLIST:
		handle_save(screen, c);
		break;
	case CMD_SCREEN_UPDATE:
		screen->painted = 0;
		browser.lw->clear = 1;
		browser.lw->repaint = 1;
		browser.filelist = mpdclient_filelist_update(c, browser.filelist);
		list_window_check_selected(browser.lw,
					   browser.filelist->length);
		screen_status_printf(_("Screen updated!"));
		return 1;
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
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		return screen_find(screen,
				   browser.lw, browser.filelist->length,
				   cmd, browse_lw_callback,
				   browser.filelist);
	case CMD_MOUSE_EVENT:
		return browse_handle_mouse_event(screen,c, browser.lw,
						 browser.filelist);
	default:
		break;
	}

	return list_window_cmd(browser.lw, browser.filelist->length, cmd);
}

const struct screen_functions screen_browse = {
	.init = browse_init,
	.exit = browse_exit,
	.open = browse_open,
	.resize = browse_resize,
	.paint = browse_paint,
	.update = browse_update,
	.cmd = browse_cmd,
	.get_title = browse_title,
};

