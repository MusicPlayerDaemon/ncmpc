/*
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
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

#include "screen_browser.h"
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "strfsong.h"
#include "screen_utils.h"
#include "gcc.h"

#include <string.h>

#define USE_OLD_LAYOUT
#undef  USE_OLD_ADD

#define BUFSIZE 1024

#define HIGHLIGHT  (0x01)

/* clear the highlight flag for all items in the filelist */
static void
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
static void
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

/* the playlist have been updated -> fix highlights */
void
browser_playlist_changed(struct screen_browser *browser, mpdclient_t *c,
			 int event, gpointer data)
{
	if (browser->filelist == NULL)
		return;

	D("screen_file.c> playlist_callback() [%d]\n", event);
	switch(event) {
	case PLAYLIST_EVENT_CLEAR:
		clear_highlights(browser->filelist);
		break;
	case PLAYLIST_EVENT_ADD:
		set_highlight(browser->filelist, (mpd_Song *) data, 1);
		break;
	case PLAYLIST_EVENT_DELETE:
		set_highlight(browser->filelist, (mpd_Song *) data, 0);
		break;
	case PLAYLIST_EVENT_MOVE:
		break;
	default:
		sync_highlights(c, browser->filelist);
		break;
	}
}

/* list_window callback */
const char *
browser_lw_callback(unsigned idx, int *highlight, void *data)
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
int
browser_change_directory(struct screen_browser *browser, mpdclient_t *c,
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
			char *parent = g_path_get_dirname(browser->filelist->path);
			if( strcmp(parent, ".") == 0 )
				parent[0] = '\0';
			path = g_strdup(parent);
			list_window_reset(browser->lw);
			/* restore previous list window state */
			list_window_pop_state(browser->lw_state, browser->lw);
		} else {
			/* entry==NULL, then new_path ("" is root) */
			path = g_strdup(new_path);
			list_window_reset(browser->lw);
			/* restore first list window state (pop while returning true) */
			while(list_window_pop_state(browser->lw_state, browser->lw));
		}
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		/* enter sub */
		mpd_Directory *dir = entity->info.directory;
		path = utf8_to_locale(dir->path);
		/* save current list window state */
		list_window_push_state(browser->lw_state, browser->lw);
	} else
		return -1;

	mpdclient_filelist_free(browser->filelist);
	browser->filelist = mpdclient_filelist_get(c, path);
	sync_highlights(c, browser->filelist);
	list_window_check_selected(browser->lw, browser->filelist->length);
	g_free(path);
	return 0;
}

static int
load_playlist(mpdclient_t *c, filelist_entry_t *entry)
{
	mpd_InfoEntity *entity = entry->entity;
	mpd_PlaylistFile *plf = entity->info.playlistFile;
	char *filename = utf8_to_locale(plf->path);

	if (mpdclient_cmd_load_playlist_utf8(c, plf->path) == 0)
		screen_status_printf(_("Loading playlist %s..."), basename(filename));
	g_free(filename);
	return 0;
}

static int
enqueue_and_play(mpdclient_t *c, filelist_entry_t *entry)
{
	int idx;
	mpd_InfoEntity *entity = entry->entity;
	mpd_Song *song = entity->info.song;

	if (!(entry->flags & HIGHLIGHT)) {
		if (mpdclient_cmd_add(c, song) == 0) {
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
browser_handle_enter(struct screen_browser *browser, mpdclient_t *c)
{
	filelist_entry_t *entry;
	mpd_InfoEntity *entity;

	if (browser->filelist == NULL)
		return -1;
	entry = (filelist_entry_t *) g_list_nth_data(browser->filelist->list,
						     browser->lw->selected);
	if( entry==NULL )
		return -1;

	entity = entry->entity;
	if (entity == NULL || entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		return browser_change_directory(browser, c, entry, NULL);
	else if (entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		return load_playlist(c, entry);
	else if (entity->type == MPD_INFO_ENTITY_TYPE_SONG)
		return enqueue_and_play(c, entry);
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

static int
browser_select_entry(mpdclient_t *c, filelist_entry_t *entry,
		     gboolean toggle)
{
	assert(entry != NULL);
	assert(entry->entity != NULL);

	if (entry->entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		return load_playlist(c, entry);

	if (entry->entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
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
		return 0;
	}

	if (entry->entity->type != MPD_INFO_ENTITY_TYPE_SONG)
		return -1;

	if (toggle && entry->flags & HIGHLIGHT)
		entry->flags &= ~HIGHLIGHT;
	else
		entry->flags |= HIGHLIGHT;

	if (toggle || entry->flags & HIGHLIGHT) {
		if (entry->entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_Song *song = entry->entity->info.song;

			if (mpdclient_cmd_add(c, song) == 0) {
				char buf[BUFSIZE];

				strfsong(buf, BUFSIZE, LIST_FORMAT, song);
				screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
			}
		}
	} else {
		/* remove song from playlist */
		if (entry->entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_Song *song = entry->entity->info.song;

			if (song) {
				int idx;

				while ((idx = playlist_get_index_from_file(c, song->file)) >=0)
					mpdclient_cmd_delete(c, idx);
			}
		}
	}

	return 0;
}

int
browser_handle_select(struct screen_browser *browser, mpdclient_t *c)
{
	filelist_entry_t *entry;

	if (browser->filelist == NULL)
		return -1;
	entry = g_list_nth_data(browser->filelist->list, browser->lw->selected);
	if (entry == NULL || entry->entity == NULL)
		return -1;

	return browser_select_entry(c, entry, TRUE);
}

void
browser_handle_select_all(struct screen_browser *browser, mpdclient_t *c)
{
	filelist_entry_t *entry;
	GList *temp = browser->filelist->list;

	if (browser->filelist == NULL)
		return;

	for (browser->filelist->list = g_list_first(browser->filelist->list);
	     browser->filelist->list;
	     browser->filelist->list = g_list_next(browser->filelist->list)) {
		entry = browser->filelist->list->data;
		if (entry != NULL && entry->entity != NULL)
			browser_select_entry(c, entry, FALSE);
	}

	browser->filelist->list = temp;
}

#ifdef HAVE_GETMOUSE
int
browser_handle_mouse_event(struct screen_browser *browser, mpdclient_t *c)
{
	int row;
	unsigned prev_selected = browser->lw->selected;
	unsigned long bstate;
	int length;

	if (browser->filelist)
		length = browser->filelist->length;
	else
		length = 0;

	if( screen_get_mouse_event(c, browser->lw, length, &bstate, &row) )
		return 1;

	browser->lw->selected = browser->lw->start + row;
	list_window_check_selected(browser->lw, length);

	if( bstate & BUTTON1_CLICKED ) {
		if (prev_selected == browser->lw->selected)
			browser_handle_enter(browser, c);
	} else if (bstate & BUTTON3_CLICKED) {
		if (prev_selected == browser->lw->selected)
			browser_handle_select(browser, c);
	}

	return 1;
}
#endif

