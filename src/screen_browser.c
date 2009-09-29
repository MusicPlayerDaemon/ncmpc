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

#include "screen_browser.h"
#include "i18n.h"
#include "options.h"
#include "charset.h"
#include "strfsong.h"
#include "screen_utils.h"
#include "mpdclient.h"
#include "filelist.h"

#include <mpd/client.h>

#include <string.h>

#define BUFSIZE 1024

#ifndef NCMPC_MINI
#define HIGHLIGHT  (0x01)
#endif

static const char playlist_format[] = "*%s*";

#ifndef NCMPC_MINI

/* clear the highlight flag for all items in the filelist */
static void
clear_highlights(struct filelist *fl)
{
	guint i;

	for (i = 0; i < filelist_length(fl); ++i) {
		struct filelist_entry *entry = filelist_get(fl, i);

		entry->flags &= ~HIGHLIGHT;
	}
}

/* change the highlight flag for a song */
static void
set_highlight(struct filelist *fl, struct mpd_song *song, int highlight)
{
	int i = filelist_find_song(fl, song);
	struct filelist_entry *entry;

	if (i < 0)
		return;

	entry = filelist_get(fl, i);
	if (highlight)
		entry->flags |= HIGHLIGHT;
	else
		entry->flags &= ~HIGHLIGHT;
}

/* sync highlight flags with playlist */
void
sync_highlights(struct mpdclient *c, struct filelist *fl)
{
	guint i;

	for (i = 0; i < filelist_length(fl); ++i) {
		struct filelist_entry *entry = filelist_get(fl, i);
		struct mpd_entity *entity = entry->entity;

		if (entity != NULL && mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			const struct mpd_song *song =
				mpd_entity_get_song(entity);

			if (playlist_get_index_from_same_song(&c->playlist,
							      song) >= 0)
				entry->flags |= HIGHLIGHT;
			else
				entry->flags &= ~HIGHLIGHT;
		}
	}
}

/* the playlist has been updated -> fix highlights */
void
browser_playlist_changed(struct screen_browser *browser, struct mpdclient *c,
			 int event, gpointer data)
{
	if (browser->filelist == NULL)
		return;

	switch(event) {
	case PLAYLIST_EVENT_CLEAR:
		clear_highlights(browser->filelist);
		break;
	case PLAYLIST_EVENT_ADD:
		set_highlight(browser->filelist, (struct mpd_song *) data, 1);
		break;
	case PLAYLIST_EVENT_DELETE:
		set_highlight(browser->filelist, (struct mpd_song *) data, 0);
		break;
	case PLAYLIST_EVENT_MOVE:
		break;
	default:
		sync_highlights(c, browser->filelist);
		break;
	}
}

#endif

/* list_window callback */
const char *
browser_lw_callback(unsigned idx, bool *highlight, G_GNUC_UNUSED char **second_column, void *data)
{
	struct filelist *fl = (struct filelist *) data;
	static char buf[BUFSIZE];
	struct filelist_entry *entry;
	struct mpd_entity *entity;

	if (fl == NULL || idx >= filelist_length(fl))
		return NULL;

	entry = filelist_get(fl, idx);
	assert(entry != NULL);

	entity = entry->entity;
#ifndef NCMPC_MINI
	*highlight = (entry->flags & HIGHLIGHT) != 0;
#else
	*highlight = false;
#endif

	if( entity == NULL )
		return "[..]";

	if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const struct mpd_directory *dir =
			mpd_entity_get_directory(entity);
		char *directory = utf8_to_locale(g_basename(mpd_directory_get_path(dir)));

		g_snprintf(buf, BUFSIZE, "[%s]", directory);
		g_free(directory);
		return buf;
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
		const struct mpd_song *song = mpd_entity_get_song(entity);

		strfsong(buf, BUFSIZE, options.list_format, song);
		return buf;
	} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
		const struct mpd_playlist *playlist =
			mpd_entity_get_playlist(entity);
		char *filename = utf8_to_locale(g_basename(mpd_playlist_get_path(playlist)));

		g_snprintf(buf, BUFSIZE, playlist_format, filename);
		g_free(filename);
		return buf;
	}

	return "Error: Unknown entry!";
}

static bool
load_playlist(struct mpdclient *c, const struct mpd_playlist *playlist)
{
	char *filename = utf8_to_locale(mpd_playlist_get_path(playlist));

	if (mpdclient_cmd_load_playlist(c, mpd_playlist_get_path(playlist)) == 0)
		screen_status_printf(_("Loading playlist %s..."),
				     g_basename(filename));
	g_free(filename);
	return true;
}

static bool
enqueue_and_play(struct mpdclient *c, struct filelist_entry *entry)
{
	int idx;
	const struct mpd_song *song = mpd_entity_get_song(entry->entity);

#ifndef NCMPC_MINI
	if (!(entry->flags & HIGHLIGHT)) {
#endif
		if (mpdclient_cmd_add(c, song) == 0) {
			char buf[BUFSIZE];

#ifndef NCMPC_MINI
			entry->flags |= HIGHLIGHT;
#endif
			strfsong(buf, BUFSIZE, options.list_format, song);
			screen_status_printf(_("Adding \'%s\' to playlist"), buf);
			mpdclient_update(c); /* get song id */
		} else
			return false;
#ifndef NCMPC_MINI
	}
#endif

	idx = playlist_get_index_from_same_song(&c->playlist, song);
	mpdclient_cmd_play(c, idx);
	return true;
}

struct filelist_entry *
browser_get_selected_entry(const struct screen_browser *browser)
{
	if (browser->filelist == NULL ||
	    browser->lw->selected_start < browser->lw->selected_end ||
	    browser->lw->selected >= filelist_length(browser->filelist))
		return NULL;

	return filelist_get(browser->filelist, browser->lw->selected);
}

static const struct mpd_entity *
browser_get_selected_entity(const struct screen_browser *browser)
{
	const struct filelist_entry *entry = browser_get_selected_entry(browser);

	return entry != NULL
		? entry->entity
		: NULL;
}

static const struct mpd_song *
browser_get_selected_song(const struct screen_browser *browser)
{
	const struct mpd_entity *entity = browser_get_selected_entity(browser);

	return entity != NULL &&
		mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG
		? mpd_entity_get_song(entity)
		: NULL;
}

static struct filelist_entry *
browser_get_index(const struct screen_browser *browser, unsigned i)
{
	if (browser->filelist == NULL ||
	    i >= filelist_length(browser->filelist))
		return NULL;

	return filelist_get(browser->filelist, i);
}

static bool
browser_handle_enter(struct screen_browser *browser, struct mpdclient *c)
{
	struct filelist_entry *entry = browser_get_selected_entry(browser);
	struct mpd_entity *entity;

	if (entry == NULL)
		return false;

	entity = entry->entity;
	if (entity == NULL)
		return false;

	if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST)
		return load_playlist(c, mpd_entity_get_playlist(entity));
	else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
		return enqueue_and_play(c, entry);
	return false;
}

static bool
browser_select_entry(struct mpdclient *c, struct filelist_entry *entry,
		     G_GNUC_UNUSED gboolean toggle)
{
	assert(entry != NULL);
	assert(entry->entity != NULL);

	if (mpd_entity_get_type(entry->entity) == MPD_ENTITY_TYPE_PLAYLIST)
		return load_playlist(c, mpd_entity_get_playlist(entry->entity));

	if (mpd_entity_get_type(entry->entity) == MPD_ENTITY_TYPE_DIRECTORY) {
		const struct mpd_directory *dir =
			mpd_entity_get_directory(entry->entity);

		if (mpdclient_cmd_add_path(c, mpd_directory_get_path(dir)) == 0) {
			char *tmp = utf8_to_locale(mpd_directory_get_path(dir));

			screen_status_printf(_("Adding \'%s\' to playlist"), tmp);
			g_free(tmp);
		}

		return true;
	}

	if (mpd_entity_get_type(entry->entity) != MPD_ENTITY_TYPE_SONG)
		return false;

#ifndef NCMPC_MINI
	if (!toggle || (entry->flags & HIGHLIGHT) == 0)
#endif
	{
		const struct mpd_song *song =
			mpd_entity_get_song(entry->entity);

#ifndef NCMPC_MINI
		entry->flags |= HIGHLIGHT;
#endif

		if (mpdclient_cmd_add(c, song) == 0) {
			char buf[BUFSIZE];

			strfsong(buf, BUFSIZE, options.list_format, song);
			screen_status_printf(_("Adding \'%s\' to playlist"), buf);
		}
#ifndef NCMPC_MINI
	} else {
		/* remove song from playlist */
		const struct mpd_song *song =
			mpd_entity_get_song(entry->entity);
		int idx;

		entry->flags &= ~HIGHLIGHT;

		while ((idx = playlist_get_index_from_same_song(&c->playlist,
								song)) >= 0)
			mpdclient_cmd_delete(c, idx);
#endif
	}

	return true;
}

static bool
browser_handle_select(struct screen_browser *browser, struct mpdclient *c)
{
	struct filelist_entry *entry;

	if (browser->lw->range_selection) {
		for (unsigned i = browser->lw->selected_start;
		         i <= browser->lw->selected_end; i++) {
			entry = browser_get_index(browser, i);

			if (entry != NULL && entry->entity != NULL)
				browser_select_entry(c, entry, TRUE);
		}
		return false;
	} else {
		entry = browser_get_selected_entry(browser);

		if (entry == NULL || entry->entity == NULL)
			return false;

		return browser_select_entry(c, entry, TRUE);
	}
}

static bool
browser_handle_add(struct screen_browser *browser, struct mpdclient *c)
{
	struct filelist_entry *entry;

	if (browser->lw->range_selection) {
		for (unsigned i = browser->lw->selected_start;
		         i <= browser->lw->selected_end; i++) {
			entry = browser_get_index(browser, i);

			if (entry != NULL && entry->entity != NULL)
				browser_select_entry(c, entry, FALSE);
		}
		return false;
	} else {
		entry = browser_get_selected_entry(browser);

		if (entry == NULL || entry->entity == NULL)
			return false;

		return browser_select_entry(c, entry, FALSE);
	}
}

static void
browser_handle_select_all(struct screen_browser *browser, struct mpdclient *c)
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
browser_handle_mouse_event(struct screen_browser *browser, struct mpdclient *c)
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
	const struct mpd_song *song;

	if (browser->filelist == NULL)
		return false;

	switch (cmd) {
	case CMD_PLAY:
		browser_handle_enter(browser, c);
		return true;

	case CMD_SELECT:
		if (browser_handle_select(browser, c))
			/* continue and select next item... */
			cmd = CMD_LIST_NEXT;

		/* call list_window_cmd to go to the next item */
		break;

	case CMD_ADD:
		if (browser_handle_add(browser, c))
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
	case CMD_LIST_JUMP:
		screen_jump(browser->lw, browser_lw_callback, browser->filelist);
		return true;

#ifdef HAVE_GETMOUSE
	case CMD_MOUSE_EVENT:
		browser_handle_mouse_event(browser, c);
		return true;
#endif

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		song = browser_get_selected_song(browser);
		if (song == NULL)
			return false;

		screen_song_switch(c, song);
		return true;
#endif

	case CMD_LOCATE:
		song = browser_get_selected_song(browser);
		if (song == NULL)
			return false;

		screen_file_goto_song(c, song);
		return true;

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		song = browser_get_selected_song(browser);
		if (song == NULL)
			return false;

		screen_lyrics_switch(c, song, false);
		return true;
#endif
	case CMD_SCREEN_SWAP:
		screen_swap(c, browser_get_selected_song(browser));
		return true;

	default:
		break;
	}

	if (list_window_cmd(browser->lw, filelist_length(browser->filelist),
			    cmd))
		return true;

	return false;
}
