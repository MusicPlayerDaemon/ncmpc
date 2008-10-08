/*
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
#include "i18n.h"
#include "options.h"
#include "charset.h"
#include "strfsong.h"
#include "screen_utils.h"
#include "gcc.h"

#include <string.h>

#undef  USE_OLD_ADD

#define BUFSIZE 1024

#define HIGHLIGHT  (0x01)

static const char playlist_format[] = "*%s*";

/* clear the highlight flag for all items in the filelist */
static void
clear_highlights(mpdclient_filelist_t *fl)
{
	guint i;

	for (i = 0; i < filelist_length(fl); ++i) {
		struct filelist_entry *entry = filelist_get(fl, i);

		entry->flags &= ~HIGHLIGHT;
	}
}

/* change the highlight flag for a song */
static void
set_highlight(mpdclient_filelist_t *fl, mpd_Song *song, int highlight)
{
	struct filelist_entry *entry = filelist_find_song(fl, song);
	mpd_InfoEntity *entity;

	if (entry == NULL)
		return;

	entity = entry->entity;
	if (highlight)
		entry->flags |= HIGHLIGHT;
	else
		entry->flags &= ~HIGHLIGHT;
}

/* sync highlight flags with playlist */
void
sync_highlights(mpdclient_t *c, mpdclient_filelist_t *fl)
{
	guint i;

	for (i = 0; i < filelist_length(fl); ++i) {
		struct filelist_entry *entry = filelist_get(fl, i);
		mpd_InfoEntity *entity = entry->entity;

		if ( entity && entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
			mpd_Song *song = entity->info.song;

			if( playlist_get_index_from_file(c, song->file) >= 0 )
				entry->flags |= HIGHLIGHT;
			else
				entry->flags &= ~HIGHLIGHT;
		}
	}
}

/* the playlist have been updated -> fix highlights */
void
browser_playlist_changed(struct screen_browser *browser, mpdclient_t *c,
			 int event, gpointer data)
{
	if (browser->filelist == NULL)
		return;

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

	if (idx >= filelist_length(fl))
		return NULL;

	entry = filelist_get(fl, idx);
	assert(entry != NULL);

	entity = entry->entity;
	*highlight = (entry->flags & HIGHLIGHT);

	if( entity == NULL )
		return "[..]";

	if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY ) {
		mpd_Directory *dir = entity->info.directory;
		char *directory = utf8_to_locale(g_basename(dir->path));

		g_snprintf(buf, BUFSIZE, "[%s]", directory);
		g_free(directory);
		return buf;
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_SONG ) {
		mpd_Song *song = entity->info.song;

		strfsong(buf, BUFSIZE, options.list_format, song);
		return buf;
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_PLAYLISTFILE ) {
		mpd_PlaylistFile *plf = entity->info.playlistFile;
		char *filename = utf8_to_locale(g_basename(plf->path));

		g_snprintf(buf, BUFSIZE, playlist_format, filename);
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
	char *old_path;
	int idx;

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
		} else {
			/* entry==NULL, then new_path ("" is root) */
			path = g_strdup(new_path);
		}
	} else if( entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		/* enter sub */
		mpd_Directory *dir = entity->info.directory;
		path = utf8_to_locale(dir->path);
	} else
		return -1;

	old_path = g_strdup(browser->filelist->path);

	filelist_free(browser->filelist);
	browser->filelist = mpdclient_filelist_get(c, path);
	sync_highlights(c, browser->filelist);

	idx = filelist_find_directory(browser->filelist, old_path);
	g_free(old_path);

	list_window_reset(browser->lw);
	if (idx >= 0) {
		list_window_set_selected(browser->lw, idx);
		list_window_center(browser->lw,
				   filelist_length(browser->filelist), idx);
	}

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
		screen_status_printf(_("Loading playlist %s..."),
				     g_basename(filename));
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
			strfsong(buf, BUFSIZE, options.list_format, song);
			screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
			mpdclient_update(c); /* get song id */
		} else
			return -1;
	}

	idx = playlist_get_index_from_file(c, song->file);
	mpdclient_cmd_play(c, idx);
	return 0;
}

static struct filelist_entry *
browser_get_selected(const struct screen_browser *browser)
{
	if (browser->filelist == NULL ||
	    browser->lw->selected >= filelist_length(browser->filelist))
		return NULL;

	return filelist_get(browser->filelist, browser->lw->selected);
}

static int
browser_handle_enter(struct screen_browser *browser, mpdclient_t *c)
{
	struct filelist_entry *entry = browser_get_selected(browser);
	mpd_InfoEntity *entity;

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

	assert(entry->entity->info.song != NULL);

	if (!toggle || (entry->flags & HIGHLIGHT) == 0) {
		mpd_Song *song = entry->entity->info.song;

		entry->flags |= HIGHLIGHT;

		if (mpdclient_cmd_add(c, song) == 0) {
			char buf[BUFSIZE];

			strfsong(buf, BUFSIZE, options.list_format, song);
			screen_status_printf(_("Adding \'%s\' to playlist\n"), buf);
		}
	} else {
		/* remove song from playlist */
		mpd_Song *song = entry->entity->info.song;
		int idx;

		entry->flags &= ~HIGHLIGHT;

		while ((idx = playlist_get_index_from_file(c, song->file)) >=0)
			mpdclient_cmd_delete(c, idx);
	}

	return 0;
}

static int
browser_handle_select(struct screen_browser *browser, mpdclient_t *c)
{
	struct filelist_entry *entry = browser_get_selected(browser);

	if (entry == NULL || entry->entity == NULL)
		return -1;

	return browser_select_entry(c, entry, TRUE);
}

static int
browser_handle_add(struct screen_browser *browser, mpdclient_t *c)
{
	struct filelist_entry *entry = browser_get_selected(browser);

	if (entry == NULL || entry->entity == NULL)
		return -1;

	return browser_select_entry(c, entry, FALSE);
}

static void
browser_handle_select_all(struct screen_browser *browser, mpdclient_t *c)
{
	guint i;

	if (browser->filelist == NULL)
		return;

	for (i = 0; i < filelist_length(browser->filelist); ++i) {
		struct filelist_entry *entry = filelist_get(browser->filelist, i);

		if (entry != NULL && entry->entity != NULL)
			browser_select_entry(c, entry, FALSE);
	}
}

#ifdef HAVE_GETMOUSE
static int
browser_handle_mouse_event(struct screen_browser *browser, mpdclient_t *c)
{
	int row;
	unsigned prev_selected = browser->lw->selected;
	unsigned long bstate;
	int length;

	if (browser->filelist)
		length = filelist_length(browser->filelist);
	else
		length = 0;

	if (screen_get_mouse_event(c, &bstate, &row) ||
	    list_window_mouse(browser->lw, length, bstate, row))
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

bool
browser_cmd(struct screen_browser *browser,
	    struct mpdclient *c, command_t cmd)
{
#ifdef ENABLE_LYRICS_SCREEN
	struct filelist_entry *entry;
#endif

	switch (cmd) {
	case CMD_PLAY:
		browser_handle_enter(browser, c);
		return true;

	case CMD_SELECT:
		if (browser_handle_select(browser, c) == 0)
			/* continue and select next item... */
			cmd = CMD_LIST_NEXT;

		/* call list_window_cmd to go to the next item */
		break;

	case CMD_ADD:
		if (browser_handle_add(browser, c) == 0)
			/* continue and select next item... */
			cmd = CMD_LIST_NEXT;

		/* call list_window_cmd to go to the next item */
		break;

	case CMD_SELECT_ALL:
		browser_handle_select_all(browser, c);
		return true;

	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		screen_find(browser->lw, filelist_length(browser->filelist),
			    cmd, browser_lw_callback,
			    browser->filelist);
		return true;

#ifdef HAVE_GETMOUSE
	case CMD_MOUSE_EVENT:
		browser_handle_mouse_event(browser, c);
		return true;
#endif

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		entry = browser_get_selected(browser);
		if (entry == NULL)
			return false;

		if (entry->entity == NULL ||
		    entry->entity->type != MPD_INFO_ENTITY_TYPE_SONG)
			return true;

		screen_lyrics_switch(c, entry->entity->info.song);
		return true;
#endif

	default:
		break;
	}

	if (list_window_cmd(browser->lw, filelist_length(browser->filelist),
			    cmd))
		return true;

	return false;
}
